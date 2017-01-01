/* props/tmobile_usa-1.21: TMOUS: T-MOB010 */
static bool is_variant_tmobile_usa(std::string bootcid) {
    if (HAS_SUBSTRING(bootcid, "T-MOB010")) return true;
    return false;
}

static const char *htc_tmobile_usa_properties = 
"ro.build.fingerprint=htc/pmewl_00531/htc_pmewl:6.0.1/MMB29M/737608.1:user/release-keys\n"
"ro.build.product=htc_pmewl\n"
"ro.product.device=htc_pmewl\n"
"ro.product.model=HTC 10\n"
"ro.product.model=MSM8996 for arm64\n"
"ro.telephony.ipv6_capability=1\n"
"ro.phone.min_match=10\n"
"ro.ril.vmail.310260=+18056377243\n"
"ro.ril.oem.ecclist=911\n"
"ro.ril.enable.a52=0\n"
"ro.ril.enable.a53=1\n"
"ro.ril.enable.dtm=0\n"
"ro.ril.enable.gea3=0\n"
"ro.ril.enable.amr.wideband=1\n"
"ro.ril.gprsclass=12\n"
"ro.ril.enable.sdr=0\n"
"ro.ril.enable.pre_r8fd=1\n"
"ro.ril.fd.pre_r8_tout.scr_off=17000\n"
"ro.ril.fd.pre_r8_tout.scr_on=0\n"
"ro.ril.enable.r8fd=1\n"
"ro.ril.fd.r8_tout.scr_off=2500\n"
"ro.ril.fd.r8_tout.scr_on=0\n"
"ro.ril.set.mtusize=1440\n"
"ro.ril.air.enabled=1\n"
"ro.ril.enable.ganlite=0\n"
"ro.ril.enable.isr=1\n"
"ro.ril.ltefgi=2144337586\n"
"ro.ril.lte3gpp=130\n"
"ro.ril.ltefgi.rel9=3221225472\n"
"ro.ril.ps_handover=1\n"
"ro.ril.pdpnumber.policy.roaming=3\n"
"ro.ril.hsxpa=5\n"
"ro.ril.hsdpa.category=24\n"
"ro.ril.hsupa.category=6\n"
"ro.ril.disable.cpc=0\n"
"ro.ril.oem.cnap=1\n"
"ro.ril.radio.svn=1\n"
"ro.ril.def.agps.feature=1\n"
"ro.ril.def.agps.mode=1\n"
"persist.rild.nitz_plmn=\n"
"persist.rild.nitz_long_ons_0=\n"
"persist.rild.nitz_long_ons_1=\n"
"persist.rild.nitz_long_ons_2=\n"
"persist.rild.nitz_long_ons_3=\n"
"persist.rild.nitz_short_ons_0=\n"
"persist.rild.nitz_short_ons_1=\n"
"persist.rild.nitz_short_ons_2=\n"
"persist.rild.nitz_short_ons_3=\n"
"ril.subscription.types=NV,RUIM\n"
"telephony.lteOnCdmaDevice=1\n"
"telephony.lteOnCdmaDevice=0\n"
"persist.radio.fill_eons=1\n"
"persist.radio.custom_ecc=1\n"
"persist.radio.apm_sim_not_pwdn=0\n"
"persist.radio.apm_mdm_not_pwdn=1\n"
"ro.telephony.default_network=9\n"
"persist.radio.VT_ENABLE=1\n"
"persist.radio.VT_HYBRID_ENABLE=1\n"
"persist.radio.ROTATION_ENABLE=1\n"
"persist.radio.RATE_ADAPT_ENABLE=1\n"
"persist.radio.videopause.mode=1\n"
"telephony.lteOnCdmaDevice=0\n"
;
