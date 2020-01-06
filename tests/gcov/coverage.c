/* ------------------------------------------
 * Copyright (c) 2019, Synopsys, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * 1) Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.

 * 3) Neither the name of the Synopsys, Inc., nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   --------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "embARC.h"
#include "embARC_debug.h"
#include "coverage.h"

static struct gcov_info *gcov_info_head;

/**
 * Is called by gcc-generated constructor code for each object file compiled
 * with -fprofile-arcs.
 */
void __gcov_init(struct gcov_info *info)
{
	info->next = gcov_info_head;
	gcov_info_head = info;
}

void __gcov_merge_add(gcov_type *counters, uint32_t n_counters)
{
	/* Unused. */
}

/**
 * buff_write_u64 - Store 64 bit data on a buffer and return the size
 */
#define MASK_32BIT (0xffffffffUL)
static inline void buff_write_u64(void *buffer, size_t *off, uint64_t v)
{
	*((uint32_t *)((uint8_t *)buffer + *off) + 0) = (uint32_t)(v & MASK_32BIT);
	*((uint32_t *)((uint8_t *)buffer + *off) + 1) = (uint32_t)((v >> 32) &
								   MASK_32BIT);
	*off = *off + sizeof(uint64_t);
}

/**
 * buff_write_u32 - Store 32 bit data on a buffer and return the size
 */
static inline void buff_write_u32(void *buffer, size_t *off, uint32_t v)
{
	*((uint32_t *)((uint8_t *)buffer + *off)) = v;
	*off = *off + sizeof(uint32_t);
}

static size_t calculate_buff_size(struct gcov_info *info)
{
	uint32_t iter;
	uint32_t iter_1;
	uint32_t iter_2;
	uint32_t size = sizeof(uint32_t) * 3;

	for (iter = 0U; iter < info->n_functions; iter++) {
		size += (sizeof(uint32_t) * 5);

		for (iter_1 = 0U; iter_1 < GCOV_COUNTERS; iter_1++) {
			if (!info->merge[iter_1]) {
				continue;
			}

			size += (sizeof(uint32_t) * 2);

			for (iter_2 = 0U;
			     iter_2 < info->functions[iter]->ctrs->num;
			     iter_2++) {

				size += (sizeof(uint64_t));
			}
		}
	}
	return size;
}

/**
 * populate_buffer - convert from gcov data set (info) to
 * .gcda file format.
 * This buffer will now have info similar to a regular gcda
 * format.
 */
static size_t populate_buffer(uint8_t *buffer, struct gcov_info *info)
{
	struct gcov_fn_info *functions;
	struct gcov_ctr_info *counters_per_func;
	uint32_t iter_functions;
	uint32_t iter_counts;
	uint32_t iter_counter_values;
	size_t buffer_write_position = 0;

	buff_write_u32(buffer,
		       &buffer_write_position,
		       GCOV_DATA_MAGIC);

	buff_write_u32(buffer,
		       &buffer_write_position,
		       info->version);

	buff_write_u32(buffer,
		       &buffer_write_position,
		       info->stamp);

	for (iter_functions = 0U;
	     iter_functions < info->n_functions;
	     iter_functions++) {

		functions = info->functions[iter_functions];

		buff_write_u32(buffer,
			       &buffer_write_position,
			       GCOV_TAG_FUNCTION);

		buff_write_u32(buffer,
			       &buffer_write_position,
			       GCOV_TAG_FUNCTION_LENGTH);

		buff_write_u32(buffer,
			       &buffer_write_position,
			       functions->ident);

		buff_write_u32(buffer,
			       &buffer_write_position,
			       functions->lineno_checksum);

		buff_write_u32(buffer,
			       &buffer_write_position,
			       functions->cfg_checksum);

		counters_per_func = functions->ctrs;

		for (iter_counts = 0U;
		     iter_counts < GCOV_COUNTERS;
		     iter_counts++) {

			if (!info->merge[iter_counts]) {
				continue;
			}

			buff_write_u32(buffer,
				       &buffer_write_position,
				       GCOV_TAG_FOR_COUNTER(iter_counts));

			buff_write_u32(buffer,
				       &buffer_write_position,
				       counters_per_func->num * 2U);

			for (iter_counter_values = 0U;
			     iter_counter_values < counters_per_func->num;
			     iter_counter_values++) {
				buff_write_u64(buffer,
					       &buffer_write_position,
					       counters_per_func-> \
					       values[iter_counter_values]);
			}
			counters_per_func++;
		}
	}
	return buffer_write_position;
}

static void dump_on_console(const char *filename, char *ptr, size_t len)
{
	uint32_t iter;

	EMBARC_PRINTF("\n%c", FILE_START_INDICATOR);
	while (*filename != '\0') {
		EMBARC_PRINTF("%c", *filename++);
	}
	EMBARC_PRINTF("%c", GCOV_DUMP_SEPARATOR);

	for (iter = 0U; iter < len; iter++) {
		EMBARC_PRINTF(" %02x", (uint8_t)*ptr++);
	}
}

/**
 * Retrieves gcov coverage data and sends it over the given interface.
 */
void __gcov_exit(void)
{
	uint8_t *buffer;
	size_t size;
	size_t written_size;
	struct gcov_info *gcov_list = gcov_info_head;

	EMBARC_PRINTF("\nGCOV_COVERAGE_DUMP_START");
	while (gcov_list) {
		size = calculate_buff_size(gcov_list);
		buffer = (uint8_t *) malloc(size);

		if (!buffer) {
			EMBARC_PRINTF("No Mem available to continue dump\n");
			break;
		}

		written_size = populate_buffer(buffer, gcov_list);
		if (written_size != size) {
			EMBARC_PRINTF("Write Error on buff\n");
			free(buffer);
			break;
		}
		dump_on_console(gcov_list->filename, (char *)buffer, size);
		free(buffer);
		gcov_list = gcov_list->next;
	}

	EMBARC_PRINTF("\nGCOV_COVERAGE_DUMP_END\n");
	return;
}
