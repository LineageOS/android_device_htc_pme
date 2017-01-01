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

#define HAS_SUBSTRING(std_str, find_str) \
        (std_str.find(find_str) != std::string::npos)

/* Device specific properties */
#include "htc-asia.h"
#include "htc-ee_uk.h"
#include "htc-europe.h"
#include "htc-mexico.h"
#include "htc-o2_uk.h"
#include "htc-taiwan.h"
#include "htc-tmobile_usa.h"
#include "htc-unlocked.h"

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

void cdma_noeri_properties(char const default_network[])
{
    property_set("ro.telephony.default_network", default_network);
    property_set("ro.ril.hsxpa", "4");
    property_set("ro.ril.enable.a53", "1");
    property_set("ro.ril.enable.gea3", "1");
    property_set("ro.ril.enable.r8fd", "1");
    property_set("ro.telephony.ipv6_capability", "1");
    property_set("ro.ril.force_eri_from_xml", "false");
}

static bool is_variant_sprint(std::string bootcid)
{
    return HAS_SUBSTRING(bootcid, "SPCS_001");
}

static void load_sprint_properties(void)
{
    property_set("ro.ril.enable.a53", "1");
    property_set("ro.ril.oem.ecclist", "911");
    cdma_noeri_properties("8");
    property_set("ro.build.fingerprint", "htc/pmewhl_00651/htc_pmewhl:6.0.1/MMB29M/761758.10:user/release-keys");
    property_set("ro.build.description", "1.80.651.10 8.0_g CL761758 release-keys");
    property_set("ro.product.device", "htc_pmewhl");
    property_set("ro.build.product", "htc_pmewhl");
    property_set("ro.product.model", "2PS64");
    property_set("ro.phone.min_match", "8");
    property_set("ro.cdma.home.operator.alpha", "Sprint");
    property_set("ro.cdma.home.operator.numeric", "310120");
    property_set("ro.home.operator.carrierid", "Chameleon");
    property_set("ro.gps.agps_provider", "1");
    property_set("telephony.lteOnCdmaDevice", "1");
    property_set("telephony.lteCdmaDevice", "1");
    property_set("ro.ril.oem.ecclist", "911,112,*911,#911");
    property_set("ro.ril.oem.show.act", "1");
    property_set("ril.subscription.types", "NV,RUIM");
    property_set("persist.radio.no_wait_for_card", "1");
    property_set("ro.telephony.default_cdma_sub", "1");
    property_set("ril.data.phone.type", "2");
    property_set("DEVICE_PROVISIONED", "1");
    property_set("ro.ril.hsdpa.category", "14");
    property_set("ro.ril.hsupa.category", "6");
    property_set("ro.ril.def.agps.mode", "6");
    property_set("ro.ril.enable.pre_r8fd", "1");
    property_set("ro.ril.enable.sdr", "0");
    property_set("ro.ril.disable.cpc", "1");
    property_set("ro.ril.set.mtusize", "1422");
    property_set("ro.ril.enable.a53", "1");
}

static bool is_variant_verizon(std::string bootcid)
{
    return HAS_SUBSTRING(bootcid, "VZW__001");
}

static void load_verizon_properties(void)
{
    property_set("ro.ril.enable.a53", "1");
    property_set("ro.ril.oem.ecclist", "911");
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
}

static void load_properties(const char *original_data)
{
    char *data;
    char *key, *value, *eol, *sol, *tmp;

    if ((data = (char *) malloc(strlen(original_data)+1)) == NULL) {
        ERROR("Out of memory!");
        return;
    }

    strcpy(data, original_data);

    sol = data;
    while ((eol = strchr(sol, '\n'))) {
        key = sol;
        *eol++ = 0;
        sol = eol;

        while (isspace(*key)) key++;
        if (*key == '#') continue;

        tmp = eol - 2;
        while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

        value = strchr(key, '=');
        if (!value) continue;
        *value++ = 0;

        tmp = value - 2;
        while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

        while (isspace(*value)) value++;

        property_set(key, value);
    }

    free(data);
}

void vendor_load_properties()
{
    std::string platform;
    std::string bootmid;
    std::string bootcid;

    platform = property_get("ro.board.platform");
    if (platform != ANDROID_TARGET)
        return;

    bootmid = property_get("ro.boot.mid");
    bootcid = property_get("ro.boot.cid");

    INFO("Found bootcid %s, bootmid %s\n", bootcid.c_str(), bootmid.c_str());

    if (is_variant_asia(bootcid)) {
        load_properties(htc_asia_properties);
    } else if (is_variant_ee_uk(bootcid)) {
        load_properties(htc_ee_uk_properties);
    } else if (is_variant_europe(bootcid)) {
        load_properties(htc_europe_properties);
    } else if (is_variant_mexico(bootcid)) {
        load_properties(htc_mexico_properties);
    } else if (is_variant_o2_uk(bootcid)) {
        load_properties(htc_o2_uk_properties);
    } else if (is_variant_sprint(bootcid)) {
        load_sprint_properties();
    } else if (is_variant_taiwan(bootcid)) {
        load_properties(htc_taiwan_properties);
    } else if (is_variant_tmobile_usa(bootcid)) {
        load_properties(htc_tmobile_usa_properties);
    } else if (is_variant_unlocked(bootcid) || HAS_SUBSTRING(bootcid, "11111111")) {
        load_properties(htc_unlocked_properties);
    } else if (is_variant_verizon(bootcid)) {
        load_verizon_properties();
    } else if (HAS_SUBSTRING(bootmid, "2PS620000")) {
        /* If we don't know the variant, fall back to europe but make it clear we did so */
        property_set("ro.build.product", "HTC 10 (unknown international)");
        property_set("ro.product.device", "HTC 10 (unknown international)");
        load_properties(htc_europe_properties);
    } else {
        property_set("ro.build.product", "HTC 10 (unknown)");
        property_set("ro.product.device", "HTC 10 (unknown)");
        load_properties(htc_unlocked_properties);
    }
}
