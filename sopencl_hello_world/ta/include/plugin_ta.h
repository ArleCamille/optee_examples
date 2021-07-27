/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2020, Open Mobile Platform LLC
 */

#ifndef PLUGIN_TA_H
#define PLUGIN_TA_H

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
//0183a595-0fd1-4037-82b0-1fc25c647744
#define PLUGIN_TA_UUID \
	{ 0x0183a595, 0x0fd1, 0x4037, \
		{ 0x82, 0xb0, 0x1f, 0xc2, 0x5c, 0x64, 0x77, 0x44 } }

/* trigger to use a plugin */
#define PLUGIN_TA_PING 0

/*
 * Interface with syslog tee-supplicant plugin
 */
//693a5d5a-0f1c-4ecf-a617-05804d5c9b0b
#define SYSLOG_PLUGIN_UUID { 0x693a5d5a, 0x0f1c, 0x4ecf, \
		{ 0xa6, 0x17, 0x05, 0x80, 0x4d, 0x5c, 0x9b, 0x0b } }
#define TO_SYSLOG_CMD 0

/* according to syslog.h */
#define LOG_EMERG 0 /* system is unusable */
#define LOG_ALERT 1 /* action must be taken immediately */
#define LOG_CRIT 2 /* critical conditions */
#define LOG_ERR 3 /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE 5 /* normal but significant condition */
#define LOG_INFO 6 /* informational */
#define LOG_DEBUG 7 /* debug-level messages */

#endif /*PLUGIN_TA_H*/
