/*
   Copyright (c) 2016, The Linux Foundation. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>

#include "vendor_init.h"
#include "property_service.h"
#include "log.h"
#include "util.h"

void cdma_properties(char const default_network[])
{
    property_set("ro.telephony.default_network", default_network);
    property_set("ro.ril.hsxpa", "4");
    property_set("ro.ril.enable.a53", "1");
    property_set("ro.ril.enable.gea3", "1");
    property_set("ro.ril.enable.r8fd", "1");
    property_set("ro.telephony.ipv6_capability", "1");
    property_set("ro.ril.force_eri_from_xml", "true");
}

void gsm_properties(char const default_network[])
{
    property_set("ro.telephony.default_network", default_network);
    property_set("ro.ril.hsdpa.category", "24");
    property_set("ro.ril.hsupa.category", "6");
    property_set("ro.ril.hsxpa", "5");
    property_set("ro.ril.air.enabled", "1");
    property_set("ro.ril.enable.a52", "0");
    property_set("ro.ril.enable.gea3", "1");
    property_set("ro.telephony.ipv6_capability", "1");
}

void vendor_load_properties()
{
    char platform[PROP_VALUE_MAX];
    char bootmid[PROP_VALUE_MAX];
    char bootcid[PROP_VALUE_MAX];
    char device[PROP_VALUE_MAX];
    char carrier[PROP_VALUE_MAX];
    int rc;

    rc = property_get("ro.board.platform", platform);
    if (!rc || strncmp(platform, ANDROID_TARGET, PROP_VALUE_MAX))
        return;

    property_get("ro.boot.mid", bootmid);
    property_get("ro.boot.cid", bootcid);

    if (strstr(bootmid, "2PS620000")) {
        /* Common props for HTC 10 (International) */
        gsm_properties("9");
        property_set("ro.build.fingerprint", "htc/pmeuhl_00401/htc_pmeuhl:6.0.1/MMB29M/738269.1:user/release-keys");
        property_set("ro.build.description", "1.30.401.1 8.0_g CL738269 release-keys");
        property_set("ro.product.device", "htc_pmeuhl");
        property_set("ro.build.product", "htc_pmeuhl");
        if (strstr(bootcid, "HTC__001")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.phone.min_match", "8");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "1");
            property_set("ro.ril.air.enabled", "0");
            property_set("ro.ril.oem.ecclist", "999,112,911");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.ril.disable.cpc", "0");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.gsm.to.lte.blind.redir", "0");
            property_set("ro.ril.hsdpa.dbdc", "1");
        } else if (strstr(bootcid, "HTC__002")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.phone.min_match", "8");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "999,997,100,166,199,101,102,103,104,113,114,115,118,117");
            property_set("ro.ril.oem.normalcall.ecclist", "113,114,115,118,117");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.hsdpa.dbdc", "1");
        } else if (strstr(bootcid, "HTC__016")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.phone.min_match", "8");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "112,911,999,100");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.disable.cpc", "0");
            property_set("ro.ril.hsdpa.dbdc", "1");
        } else if (strstr(bootcid, "HTC__034")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "112,911");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.hsdpa.dbdc", "1");
        } else if (strstr(bootcid, "HTC__A07")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.phone.min_match", "8");
            property_set("ro.ril.enable.a53", "0");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "112,911");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.hsdpa.dbdc", "1");
        } else if (strstr(bootcid, "HTC__J15")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.phone.min_match", "7");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.hsdpa.dbdc", "1");
        } else if (strstr(bootcid, "HTC__M27")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.hsdpa.dbdc", "1");
        } else if (strstr(bootcid, "HTC__039")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.emc.mode", "1");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "000,111,112");
            property_set("ro.ril.oem.nosim.ecclist", "000,111,112");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.ltefgi", "2144337598");
            property_set("ro.ril.lte3gpp", "130");
            property_set("ro.ril.pdpnumber.policy.roaming", "3");
        } else if (strstr(bootcid, "OPTUS001")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "1");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "000,111,112");
            property_set("ro.ril.oem.nosim.ecclist", "000,111,112");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "0");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "0");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.enable.r8fd", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "0");
            property_set("ro.ril.fd.r8_tout.scr_on", "0");
            property_set("ro.ril.disable.sync_pf", "1");
        } else if (strstr(bootcid, "TELNZ001")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "000,111,112,911");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "0");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "0");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.enable.r8fd", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "0");
            property_set("ro.ril.fd.r8_tout.scr_on", "0");
        } else if (strstr(bootcid, "VODAP021")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.enable.dtm", "1");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "000,111,112");
            property_set("ro.ril.oem.nosim.ecclist", "000,111,112");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("persist.radio.spn.nw_scan", "1");
            property_set("ro.ril.pdpnumber.policy.roaming", "3");
        } else if (strstr(bootcid, "EVE__001")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "999,112,911");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "0");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "3");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "3");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "3");
            property_set("ro.ril.fd.r8_tout.scr_on", "5");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.disable.mcc.filter", "1");
            property_set("ro.ril.ltefgi", "1561329280");
            property_set("ro.ril.ltefgi.rel9", "3229614080");
            property_set("ro.ril.disable.sync_pf", "0");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
        } else if (strstr(bootcid, "O2___001")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.air.enabled", "0");
            property_set("ro.ril.oem.ecclist", "999,112,911");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "0");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "3");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.to.lte.blind.redir", "0");
            property_set("ro.ril.ltefgi", "2077228732");
            property_set("ro.ril.lte3gpp", "130");
            property_set("ro.ril.ltefgi.rel9", "3221225472");
            property_set("ro.ril.gsm.amrwb", "0");
            property_set("ro.ril.gsm.only.with.cs.only", "1");
            property_set("ro.telephony.bl", "27202");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
        } else if (strstr(bootcid, "O2___102")) {
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "112,911");
            property_set("ro.ril.enable.sdr", "0");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "0");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "3");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.ltefgi", "2144337596");
            property_set("ro.ril.lte3gpp", "130");
            property_set("ro.ril.ltefgi.rel9", "3221225472");
            property_set("ro.ril.gsm.amrwb", "1");
            property_set("ro.telephony.bl", "27202");
            property_set("ro.ril.ltefgi.rel10", "1076101120");
            property_set("ro.ril.dtmf_interval", "0");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
        } else if (strstr(bootcid, "HTC__621")) {
            /* HTC 10H (Tawian) */
            gsm_properties("9");
            property_set("ro.product.model", "HTC 10H");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "112,911,110,119");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.enable.amr.wideband", "0");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "2");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "3");
            property_set("ro.ril.fd.r8_tout.scr_off", "3");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.gsm.to.lte.blind.redir", "1");
            property_set("ro.ril.ltefgi", "2144337598");
            property_set("ro.ril.ltefgi.rel9", "3221225472");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
        }
    } else if (strstr(bootmid, "2PS650000")) {
        /* Common props for HTC 10 (USA) */
        property_set("ro.ril.enable.a53", "1");
        property_set("ro.ril.oem.ecclist", "911");
        if (strstr(bootcid, "VZW__001")) {
            /* HTC 10 (Verizon) */
            cdma_properties("10");
            property_set("ro.build.fingerprint", "htc/HTCOneM10vzw/htc_pmewl:6.0.1/MMB29M/774095.6:user/release-keys");
            property_set("ro.build.description", "1.82.605.6 8.0_g CL774095 release-keys");
            property_set("ro.product.model", "HTC6545LVW");
            property_set("ro.product.device", "htc_pmewl");
            property_set("ro.build.product", "htc_pmewl");
            property_set("telephony.lteOnCdmaDevice", "1");
            property_set("ro.ril.hsdpa.category", "24");
            property_set("ro.ril.hsupa.category", "6");
            property_set("ro.ril.enable.sdr", "0");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.cdma.home.operator.alpha", "Verizon");
            property_set("ro.cdma.home.operator.numeric", "310012");
            property_set("ro.ril.gprsclass", "12");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.set.mtusize", "1428");
            property_set("ro.ril.air.enabled", "0");
            property_set("ro.ril.gsm.to.lte.blind.redir", "0");
            property_set("ro.ril.roaming_lte.plmn", "302220,302610,45400,45402,45410,45418");
            property_set("ro.cdma.data_retry_config", "max_retries=infinite,0,0,60000,120000,480000,900000");
            property_set("ro.gsm.data_retry_config", "max_retries=infinite,0,0,60000,120000,480000,900000");
            property_set("ro.gsm.2nd_data_retry_config", "max_retries=infinite,0,0,60000,120000,480000,900000");
            property_set("ro.config.svlte1x", "true");
            property_set("ro.ril.radio.svn", "1");
            property_set("ro.ril.def.agps.mode", "1");
            property_set("ro.phone.save_timer", "10000");
            property_set("ro.ril.fast.dormancy.cdma.rule", "0");
            property_set("persist.radio.sib16_support", "1");
        } else if (strstr(bootcid, "T-MOB010")) {
            /* HTC 10 (T-Mobile) */
            gsm_properties("9");
            property_set("ro.build.fingerprint", "htc/pmewl_00531/htc_pmewl:6.0.1/MMB29M/737608.1:user/release-keys");
            property_set("ro.build.description", "1.21.531.1 8.0_g CL737608 release-keys");
            property_set("ro.product.device", "htc_pmewl");
            property_set("ro.build.product", "htc_pmewl");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.phone.min_match", "10");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "911");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "0");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.radio.svn", "1");
            property_set("ro.ril.def.agps.feature", "1");
            property_set("ro.ril.def.agps.mode", "1");
            property_set("ro.ril.ps_handover", "1");
            property_set("ro.ril.lte3gpp", "130");
            property_set("ro.ril.ltefgi.rel9", "3221225472");
            property_set("ro.ril.enable.ganlite", "0");
            property_set("ro.ril.set.mtusize", "1440");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "17000");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "2500");
            property_set("ro.ril.fd.r8_tout.scr_on", "0");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("wifi.hs20.support", "true");
        } else if (strstr(bootcid, "HTC__332")) {
            /* HTC 10 (Telus Mexico) */
            gsm_properties("9");
            property_set("ro.phone.min_match", "10");
            property_set("ro.ril.enable.dtm", "1");
            property_set("ro.ril.air.enabled", "1");
            property_set("ro.ril.oem.ecclist", "060,911,112");
            property_set("ro.ril.oem.nosim.ecclist", "060");
            property_set("ro.ril.enable.amr.wideband", "1");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.disable.cpc", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "2");
            property_set("ro.ril.fd.r8_tout.scr_on", "3");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "0");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.gprsclass", "12");
            property_set("ro.gsm.data_retry_config", "max_retries=infinite,45000");
            property_set("ro.gsm.2nd_data_retry_config", "max_retries=infinite,45000");
            property_set("ro.ril.enable.r8fd", "1");
            property_set("ro.ril.enable.pre_r8fd", "1");
        } else if (strstr(bootcid, "BS_US001") || strstr(bootcid, "BS_US002") || strstr(bootcid, "11111111")) {
            /* HTC 10 (North America Unlocked) */
            /* Use the generic-ish default props used on Canadian carriers for now */
            gsm_properties("9");
            property_set("ro.build.fingerprint", "htc/pmewl_00617/htc_pmewl:6.0.1/MMB29M/748430.5:user/release-keys");
            property_set("ro.build.description", "1.53.617.5 8.0_g CL748430 release-keys");
            property_set("ro.product.device", "htc_pmewl");
            property_set("ro.build.product", "htc_pmewl");
            property_set("ro.product.model", "HTC 10");
            property_set("ro.ril.enable.dtm", "0");
            property_set("ro.ril.oem.ecclist", "911");
            property_set("ro.ril.enable.sdr", "1");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.ril.enable.isr", "1");
            property_set("ro.ril.radio.svn", "1");
            property_set("ro.ril.ps_handover", "0");
            property_set("ro.ril.lte3gpp", "130");
            property_set("ro.ril.ltefgi.rel9", "2147483648");
            property_set("ro.ril.set.mtusize", "1430");
            property_set("ro.ril.fd.pre_r8_tout.scr_off", "3600");
            property_set("ro.ril.fd.pre_r8_tout.scr_on", "0");
            property_set("ro.ril.fd.r8_tout.scr_off", "0");
            property_set("ro.ril.fd.r8_tout.scr_on", "0");
            property_set("ro.ril.enable.r8fd", "0");
            property_set("ro.ril.enable.pre_r8fd", "1");
        }
    } else if (strstr(bootmid, "2PS640000")) {
        if (strstr(bootcid, "SPCS_001")) {
            /* HTC 10 (Sprint) */
            cdma_properties("10");
            property_set("ro.build.fingerprint", "htc/pmewhl_00651/htc_pmewhl:6.0.1/MMB29M/761758.10:user/release-keys");
            property_set("ro.build.description", "1.80.651.10 8.0_g CL761758 release-keys");
            property_set("ro.product.device", "htc_pmewhl");
            property_set("ro.build.product", "htc_pmewhl");
            property_set("ro.product.model", "2PS64");
            property_set("ro.phone.min_match", "8");
            property_set("ro.ril.hsdpa.category", "14");
            property_set("ro.ril.hsupa.category", "6");
            property_set("ro.ril.def.agps.mode", "6");
            property_set("ro.ril.enable.pre_r8fd", "1");
            property_set("ro.ril.enable.sdr", "0");
            property_set("ro.ril.disable.cpc", "1");
            property_set("ro.ril.set.mtusize", "1422");
            property_set("ro.ril.enable.a53", "1");
            property_set("ro.ril.oem.ecclist", "911");
        }
    }

    property_get("ro.product.device", device);
    INFO("Found bootcid %s, bootmid %s setting build properties for %s device\n", bootcid, bootmid, device);
}
