//
// Copyright (c) 2022 The Khronos Group Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include "harness/deviceInfo.h"
#include "harness/testHarness.h"
#include <iostream>
#include <string>
#include <vector>

typedef struct
{
    cl_uint info;
    const char *name;
} _info;

_info platform_info_table[] = {
#define STRING(x)                                                              \
    {                                                                          \
        x, #x                                                                  \
    }
    STRING(CL_PLATFORM_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR),
    STRING(CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR),
    STRING(CL_PLATFORM_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR)
#undef STRING
};

_info device_info_table[] = {
#define STRING(x)                                                              \
    {                                                                          \
        x, #x                                                                  \
    }
    STRING(CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR),
    STRING(CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR),
    STRING(CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR)
#undef STRING
};

int test_platform_info(cl_device_id deviceID, cl_context _context,
                       cl_command_queue _queue, int num_elements)
{
    cl_uint num_platforms;
    cl_uint i, j;
    cl_platform_id *platforms;
    cl_int errNum;
    cl_uint *handle_type;
    size_t handle_type_size = 0;
    cl_uint num_handles = 0;
    cl_uint num_platforms_skipped = 0;

    // get total # of platforms
    errNum = clGetPlatformIDs(0, NULL, &num_platforms);
    test_error(errNum, "clGetPlatformIDs (getting count) failed");

    platforms =
        (cl_platform_id *)malloc(num_platforms * sizeof(cl_platform_id));
    if (!platforms)
    {
        printf("error allocating memory\n");
        exit(1);
    }
    log_info("%d platforms available\n", num_platforms);
    errNum = clGetPlatformIDs(num_platforms, platforms, NULL);
    test_error(errNum, "clGetPlatformIDs (getting IDs) failed");

    for (i = 0; i < num_platforms; i++)
    {
        cl_bool external_mem_extn_available =
            is_platform_extension_available(platforms[i], "cl_khr_external_semaphore");
        cl_bool external_sema_extn_available =
            is_platform_extension_available(platforms[i], "cl_khr_external_memory");
        cl_bool supports_atleast_one_sema_query = false;

        if (!external_mem_extn_available && !external_sema_extn_available) {
            log_info("Platform %d does not support 'cl_khr_external_semaphore' "
                     "and 'cl_khr_external_memory'. Skipping the test.\n", i);
            num_platforms_skipped++;
            continue;
        }

        log_info("Platform %d (id %lu) info:\n", i, (unsigned long)platforms[i]);
        for (j = 0;
             j < sizeof(platform_info_table) / sizeof(platform_info_table[0]);
             j++)
        {
            errNum =
                clGetPlatformInfo(platforms[i], platform_info_table[j].info, 0,
                                  NULL, &handle_type_size);
            test_error(errNum, "clGetPlatformInfo failed");

            if (handle_type_size == 0) {
                if (platform_info_table[j].info == CL_PLATFORM_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR
                    && external_mem_extn_available) {
                    test_fail("External memory import handle types should be reported if "
                              "cl_khr_external_memory is available.\n");
                }
                log_info("%s not supported. Skipping the query.\n",
                    platform_info_table[j].name);
                continue;
            }

            if ((platform_info_table[j].info == CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR) || 
                (platform_info_table[j].info == CL_PLATFORM_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR)) {
                supports_atleast_one_sema_query = true;
            }

            num_handles = handle_type_size / sizeof(cl_uint);
            handle_type = (cl_uint *)malloc(handle_type_size);
            errNum =
                clGetPlatformInfo(platforms[i], platform_info_table[j].info,
                                  handle_type_size, handle_type, NULL);
            test_error(errNum, "clGetPlatformInfo failed");

            log_info("%s: \n", platform_info_table[j].name);
            while (num_handles--)
            {
                log_info("%x \n", handle_type[num_handles]);
            }
            if (handle_type)
            {
                free(handle_type);
            }
        }

        if (external_sema_extn_available && !supports_atleast_one_sema_query) {
            log_info("External semaphore import/export or both should be supported "
                     "if cl_khr_external_semaphore is available.\n");
            return TEST_FAIL;
        }
    }
    if (platforms)
    {
        free(platforms);
    }

    if (num_platforms_skipped == num_platforms) {
        return TEST_SKIPPED_ITSELF;
    }

    return TEST_PASS;
}

int test_device_info(cl_device_id deviceID, cl_context _context,
                     cl_command_queue _queue, int num_elements)
{
    cl_uint j;
    cl_uint *handle_type;
    size_t handle_type_size = 0;
    cl_uint num_handles = 0;
    cl_int errNum = CL_SUCCESS;
    cl_bool external_mem_extn_available =
        is_extension_available(deviceID, "cl_khr_external_memory");
    cl_bool external_sema_extn_available =
        is_extension_available(deviceID, "cl_khr_external_semaphore");
    cl_bool supports_atleast_one_sema_query = false;

    if (!external_mem_extn_available && !external_sema_extn_available) {
        log_info("Device does not support 'cl_khr_external_semaphore' "
                 "and 'cl_khr_external_memory'. Skipping the test.\n");
        return TEST_SKIPPED_ITSELF;
    }

    for (j = 0; j < sizeof(device_info_table) / sizeof(device_info_table[0]);
         j++)
    {
        errNum = clGetDeviceInfo(deviceID, device_info_table[j].info, 0, NULL,
                                 &handle_type_size);
        test_error(errNum, "clGetDeviceInfo failed");

        if (handle_type_size == 0) {
            if (device_info_table[j].info == CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR
                && external_mem_extn_available) {
                test_fail("External memory import handle types should be reported if "
                          "cl_khr_external_memory is available.\n");
            }
            log_info("%s not supported. Skipping the query.\n",
                device_info_table[j].name);
            continue;
        }

        if ((device_info_table[j].info == CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR) || 
            (device_info_table[j].info == CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR)) {
            supports_atleast_one_sema_query = true;
        }

        num_handles = handle_type_size / sizeof(cl_uint);
        handle_type = (cl_uint *)malloc(handle_type_size);

        errNum = clGetDeviceInfo(deviceID, device_info_table[j].info,
                                 handle_type_size, handle_type, NULL);
        test_error(errNum, "clGetDeviceInfo failed");

        log_info("%s: \n", device_info_table[j].name);
        while (num_handles--)
        {
            log_info("%x \n", handle_type[num_handles]);
        }
        if (handle_type)
        {
            free(handle_type);
        }
    }

    if (external_sema_extn_available && !supports_atleast_one_sema_query) {
        log_info("External semaphore import/export or both should be supported "
                 "if cl_khr_external_semaphore is available.\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}
