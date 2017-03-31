/*
 * Copyright (C) 2012-2016 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NXP NFCC features macros definitions
 */

#ifndef NXP_NFCC_FEATURES_H
#define NXP_NFCC_FEATURES_H
/*Flags common to all chip types*/
#define NXP_NFCC_EMPTY_DATA_PACKET              TRUE
#define GEMALTO_SE_SUPPORT                      TRUE

/*PN548C2*/
#define NXP_NFCC_I2C_READ_WRITE_IMPROVEMENT     TRUE
#define NXP_NFCC_AID_MATCHING_PLATFORM_CONFIG   TRUE
#define NXP_NFCC_DYNAMIC_DUAL_UICC              FALSE
#define NXP_NFCC_ROUTING_BLOCK_BIT_PROP         TRUE
#define NXP_NFCC_MIFARE_TIANJIN                 TRUE
#define NFC_NXP_STAT_DUAL_UICC_EXT_SWITCH       TRUE
#define NFC_NXP_STAT_DUAL_UICC_WO_EXT_SWITCH    FALSE
#define NXP_NFCC_SPI_FW_DOWNLOAD_SYNC           FALSE
#define NXP_HW_ANTENNA_LOOP4_SELF_TEST          TRUE

/*HTC PME reports 0x11 firmware*/
#define NXP_NFCC_FORCE_NCI1_0_INIT              TRUE
#endif                          /* end of #ifndef NXP_NFCC_FEATURES_H */
