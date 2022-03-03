// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020, Open Mobile Platform LLC
 */

#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* For the UUID (found in the TA's h-file(s)) */
#include <plugin_ta.h>

/* OpenCL */
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/* PTEditor */
#include "ptedit_header.h"

#define SLEEP_SEC 2
#define TA_PING_CNT 5

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

int main(void)
{
	int i = 0;
	TEEC_Result res = TEEC_SUCCESS;
	TEEC_Context ctx = { };
	TEEC_Session sess = { };
	TEEC_Operation op = { };
	TEEC_UUID uuid = PLUGIN_TA_UUID;
	uint32_t err_origin = 0;
	
	/* OpenCL variables */
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_mem memobj = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;
	
	char string[MEM_SIZE];
	
	FILE *fp;
	char fileName[] = "./hello.cl";
	char *source_str;
	size_t source_size;
	
	void *gpumem_host;
	
	/* OpenCL initialization */
	/* Load the source code containing the kernel */
	fp = fopen (fileName, "r");
	if (!fp)
	{
		fprintf (stderr, "Failed to load kernel\n");
		return 1;
	}
	source_str = (char*) malloc (MAX_SOURCE_SIZE);
	source_size = fread (source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose (fp);

	/* get platform and device info */
	ret = clGetPlatformIDs (1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs (platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
	
	/* Create OpenCL context */
	context = clCreateContext (NULL, 1, &device_id, NULL, NULL, &ret);
	if (ret)
		printf ("clCreateContext error! %d\n", ret);
	
	/* Create command queue */
	command_queue = clCreateCommandQueue (context, device_id, 0, &ret);
	
	/* Create memory buffer at the GPU end */
	memobj = clCreateBuffer (context, CL_MEM_READ_WRITE, MEM_SIZE * sizeof (char), NULL, &ret);
	
	/* Map created buffer */
	gpumem_host = clEnqueueMapBuffer (command_queue, memobj, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ,
					  0, MEM_SIZE * sizeof (char), 0, NULL, NULL, &ret);
	if (ret)
	{
		printf ("clEnqueueMapBuffer error! %d\n", ret);
	}

	/* address? */
	printf ("gpumem_host = %p\n", gpumem_host);

	/* where does gpumem_host reside? */
	if (!ptedit_init ())
	{
		ptedit_entry_t vm = ptedit_resolve (gpumem_host, 0);
		if (vm.pgd != 0)
		{
			size_t gpumem_kern = (size_t) (ptedit_cast (vm.pte, ptedit_pte_t).pfn);
			gpumem_kern *= ptedit_get_pagesize ();
			printf ("physical gpumem_host = 0x%016zX\n", gpumem_kern);
		}
		else
			puts ("ptedit failed to resolve gpumem_host");
	}
	else
		puts ("ptedit initialization failed");
	
	/* Create kernel program from the source */
	program = clCreateProgramWithSource (context, 1, (const char **)&source_str,
		(const size_t *)&source_size, &ret);
	
	/* Build kernel program */
	ret = clBuildProgram (program, 1, &device_id, NULL, NULL, NULL);
	
	/* Create OpenCL kernel */
	kernel = clCreateKernel (program, "hello", &ret);
	
	/* Set OpenCL kernel parameters */
	ret = clSetKernelArg (kernel, 0, sizeof (cl_mem), (void *)&memobj);

	/* TEE initialization */
	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code %#" PRIx32,
		     res);
	
	/* Allocate a new shared memory */
	TEEC_SharedMemory shm = {0};
	shm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	shm.size = MEM_SIZE;

	/* Open a session to the "plugin" TA */
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL,
			       NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code %#" PRIx32 "origin %#" PRIx32,
		     res, err_origin);

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes =
		TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	op.params[0].memref.parent = &shm;
	op.params[0].memref.size = shm.size;
	
	if (TEEC_AllocateSharedMemory (&ctx, &shm) != TEEC_SUCCESS)
	{
		fprintf(stderr, "Failed to allocate shared memory\n");
		goto main_err;
	}
	/* shm here is ALLOCATED, not REGISTERED. You MUST copy OpenCL data to shm. */
	
	/* Back to OpenCL... */
	/* Enqueue OpenCL kernel */
	ret = clEnqueueTask (command_queue, kernel, 0, NULL, NULL);

	/* Copy results to the shared memory */
	ret = clEnqueueReadBuffer (command_queue, memobj, CL_TRUE, 0,
		MEM_SIZE * sizeof (char), shm.buffer, 0, NULL, NULL);
	
	/* Finish Current commands */
	ret = clFlush (command_queue);
	ret = clFinish (command_queue);

	/*
	 * TA will refer to the syslog plugin to print some log messages to REE.
	 *
	 * See the plugin code in the optee-client.
	 * See the log through 'journalctl'.
	 */

	printf("Work logic: REE --> plugin TA --> syslog plugin in REE --> syslog\n");
	printf("See the log from TEE through 'journalctl'\n\n");

	res = TEEC_InvokeCommand(&sess, PLUGIN_TA_PING, &op,
				 &err_origin);

	printf("Attempt: TEEC_InvokeCommand() %s; res=%#" PRIx32 " orig=%#" PRIx32 "\n",
	       (res == TEEC_SUCCESS) ? "success" : "failed",
	       res, err_origin);

	main_err:
	/*
	 * We're done with the TA, close the session and
	 * destroy the context.
	 */

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
	
	/* OpenCL cleanup */
	ret = clReleaseKernel (kernel);
	ret = clReleaseProgram (program);
	ret = clReleaseMemObject (memobj);
	ret = clReleaseCommandQueue (command_queue);
	ret = clReleaseContext (context);
	
	free (source_str);

	return 0;
}
