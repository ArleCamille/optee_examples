// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020, Open Mobile Platform LLC
 */

#include <assert.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

/* This TA header */
#include <plugin_ta.h>

#include <string.h>
#include <stdint.h>

#if defined(__arm__) || defined(__aarch32__) || defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM)
# if defined(__GNUC__)
#  include <stdint.h>
# endif
# if defined(__ARM_NEON) || defined(_MSC_VER)
#  include <arm_neon.h>
# endif
/* GCC and LLVM Clang, but not Apple Clang */
# if defined(__GNUC__) && !defined(__apple_build_version__)
#  if defined(__ARM_ACLE) || defined(__ARM_FEATURE_CRYPTO)
#   include <arm_acle.h>
#  endif
# endif
#endif  /* ARM Headers */

static const uint8_t sbox[256] = {
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
}; // we still need an S-Box because of subkey derivation

static const uint8_t rcon[11] = {
	0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

uint32_t g(uint32_t w, int r)
{
	// assume little endian
	w = (w >> 8) | (w << 24);
	uint8_t* b = (uint8_t *)&w;
	for (int i = 0; i < 4; ++i)
		b[i] = sbox[b[i]];
	b[0] ^= rcon[r];
	
	return w;
}

void derive_subkey_128 (const uint8_t key[], uint8_t* subkeys)
{
	uint32_t temp_subkeys[4 * 11];
	uint8_t* temp_subkeys_8 = (uint8_t*) temp_subkeys;
	memcpy (temp_subkeys, key, 16);
	
	for (int i = 4; i < 44; i++)
	{
		if (i % 4 == 0)
			temp_subkeys[i] = g(temp_subkeys[i-1], i / 4);
		else
			temp_subkeys[i] = temp_subkeys[i-1];
		temp_subkeys[i] ^= temp_subkeys[i-4];
	}
	memcpy (subkeys, temp_subkeys + 4, 4 * 10 * sizeof (uint32_t));
}

void aes_encrypt_arm(const uint8_t key[], const uint8_t subkeys[], uint32_t rounds,
                     const uint8_t input[], uint8_t output[], uint32_t length)
{
	while (length >= 16)
	{
		uint8x16_t block = vld1q_u8(input);

		// AES single round encryption
		block = vaeseq_u8(block, vld1q_u8(key));
		// AES mix columns
		block = vaesmcq_u8(block);

		// AES single round encryption
		block = vaeseq_u8(block, vld1q_u8(subkeys));
		// AES mix columns
		block = vaesmcq_u8(block);

		for (unsigned int i=1; i<rounds-2; ++i)
		{
			// AES single round encryption
			block = vaeseq_u8(block, vld1q_u8(subkeys+i*16));
			// AES mix columns
			block = vaesmcq_u8(block);
		}

		// AES single round encryption
		block = vaeseq_u8(block, vld1q_u8(subkeys+(rounds-2)*16));
		// Final Add (bitwise Xor)
		block = veorq_u8(block, vld1q_u8(subkeys+(rounds-1)*16));

		vst1q_u8(output, block);

		input += 16; output += 16;
		length -= 16;
	}
}

void aes_decrypt_arm(const uint8_t key[], const uint8_t subkeys[], uint32_t rounds,
                     const uint8_t input[], uint8_t output[], uint32_t length)
{
	while (length >= 16)
	{
		uint8x16_t block = vld1q_u8(input);
		uint8x16_t rk;
		
		// AES single round decryption
		block = vaesdq_u8 (block, vld1q_u8 (subkeys + (rounds-1) * 16));
		
		for (int i = rounds - 2; i >= 0; --i)
		{
			// AES inv mix columns
			block = vaesimcq_u8 (block);
			rk = vaesimcq_u8 (vld1q_u8 (subkeys + i * 16));
			// AES single round decryption
			block = vaesdq_u8 (block, rk);
		}
		
		// final xor
		block = veorq_u8 (block, vld1q_u8 (key));

		vst1q_u8(output, block);

		input += 16; output += 16;
		length -= 16;
	}
}

TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
				    TEE_Param __maybe_unused params[4],
				    void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __unused *sess_ctx)
{
}

static TEE_Result syslog_plugin_ping(void)
{
	int n = 0;
	TEE_Result res = TEE_SUCCESS;
	static uint32_t inc_var = 0;
	char log_str[64] = { 0 };
	TEE_UUID syslog_uuid = SYSLOG_PLUGIN_UUID;

	n = snprintf(log_str, sizeof(log_str), "Hello, plugin! value = 0x%x",
		     inc_var++);
	if (n > (int)sizeof(log_str))
		return TEE_ERROR_GENERIC;

	IMSG("Push syslog plugin string \"%s\"", log_str);

	res = tee_invoke_supp_plugin(&syslog_uuid, TO_SYSLOG_CMD, LOG_INFO,
				     log_str, n, NULL);
	if (res)
		EMSG("invoke plugin failed with code 0x%x", res);

	return res;
}

TEE_Result TA_InvokeCommandEntryPoint(void __unused *sess_ctx,
				      uint32_t cmd_id, uint32_t param_types,
				      TEE_Param __unused params[4])
{
	uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	switch (cmd_id) {
	case PLUGIN_TA_PING:
		return syslog_plugin_ping();
	default:
		return TEE_ERROR_NOT_SUPPORTED;
	}
}
