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

#include "embARC_test.h"
#include "arc/arc_connect.h"

UNIT_TEST(arc_connect, ici){
	arc_connect_check_core_id();
	arc_connect_ici_generate(ARC_CONNECT_CORE_0);
	arc_connect_ici_ack(ARC_CONNECT_CORE_0);
	arc_connect_ici_read_status(ARC_CONNECT_CORE_0);
	arc_connect_ici_check_src();
}

UNIT_TEST(arc_connect, gfrc){
	uint32_t val;

	arc_connect_gfrc_clear();
	val = arc_connect_gfrc_lo_read();
	val = arc_connect_gfrc_hi_read();
	arc_connect_gfrc_enable();
	arc_connect_gfrc_disable();
	arc_connect_gfrc_core_set(0x1);
	val = arc_connect_gfrc_halt_read();
	val = arc_connect_gfrc_core_read();
}

UNIT_TEST(arc_connect, idu){
	arc_connect_idu_disable();
	UNIT_TEST_ASSERT_EQUAL(arc_connect_idu_read_enable(), 0);
	arc_connect_idu_enable();
	UNIT_TEST_ASSERT_EQUAL(arc_connect_idu_read_enable(), 1);
	arc_connect_idu_set_mode(0, ARC_CONNECT_INTRPT_TRIGGER_LEVEL, ARC_CONNECT_DISTRI_MODE_ROUND_ROBIN);
	arc_connect_idu_read_mode(0);
	arc_connect_idu_set_dest(0, 0);
	arc_connect_idu_read_dest(0);
	arc_connect_idu_gen_cirq(0);
	arc_connect_idu_ack_cirq(0);
	arc_connect_idu_check_status(0);
	arc_connect_idu_check_source(0);
	arc_connect_idu_set_mask(0, 0);
	arc_connect_idu_read_mask(0);
	arc_connect_idu_check_first(0);
	arc_connect_idu_config_irq(0, 0, 0, 0);
}

UNIT_TEST(arc_connect, ics){
	arc_connect_ics_take(0);
	arc_connect_ics_release(0);
	arc_connect_ics_force_release(0);
}

UNIT_TEST(arc_connect, icm){
	uint32_t addr = 0x0;
	uint32_t offset = 0x1;
	uint32_t data = 0xa5a5a5a5;

	arc_connect_icm_addr_set(addr);
	UNIT_TEST_ASSERT_EQUAL(arc_connect_icm_addr_read(), addr);
	arc_connect_icm_addr_offset_set(offset);
	UNIT_TEST_ASSERT_EQUAL(arc_connect_icm_addr_offset_read(), offset);
	arc_connect_icm_msg_write(data);
	UNIT_TEST_ASSERT_EQUAL(arc_connect_icm_msg_read(), data);
	arc_connect_icm_msg_inc_write(data);
	UNIT_TEST_ASSERT_EQUAL(arc_connect_icm_msg_inc_read(), data);
	arc_connect_icm_msg_imm_write(0, data);
	UNIT_TEST_ASSERT_EQUAL(arc_connect_icm_msg_imm_read(addr), data);
	arc_connect_icm_ecc_ctrl_set(0);
	arc_connect_icm_ecc_ctrl_read();
}

UNIT_TEST(arc_connect, debug){
	arc_connect_debug_reset(0);
	arc_connect_debug_halt(0);
	arc_connect_debug_run(0);
	arc_connect_debug_mask_set(0, 0);
	arc_connect_debug_mask_read(0);
	arc_connect_debug_select_set(0);
	arc_connect_debug_select_read();
	arc_connect_debug_en_read();
	arc_connect_debug_cmd_read();
	arc_connect_debug_core_read();
}

UNIT_TEST(arc_connect, pmu){
	arc_connect_pmu_pucnt_set(0);
	arc_connect_pmu_pucnt_read();
	arc_connect_pmu_rstcnt_set(0);
	arc_connect_pmu_rstcnt_read();
	arc_connect_pmu_pdccnt_set(0);
	arc_connect_pmu_pdccnt_read();
}

UNIT_TEST(arc_connect, pdm){
	arc_connect_pdm_pm_set(0, 0);
	arc_connect_pdm_pdstatus_read(0);
}