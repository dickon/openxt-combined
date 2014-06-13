/*
 * Copyright (c) 2014 Citrix Systems, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* USB policy daemon
 * Store information about devices plugged into system
 */
#include "project.h"

 struct usb_vendor_product {
  int Vid;
  int Pid;
};

struct usb_vendor_product usb_network_adapters[] = {
 { 0x05c6, 0x9211 }, { 0x05c6, 0x9212 }, { 0x03f0, 0x1f1d }, { 0x03f0, 0x201d },
 { 0x04da, 0x250d }, { 0x04da, 0x250c }, { 0x413c, 0x8172 }, { 0x413c, 0x8171 },
 { 0x1410, 0xa001 }, { 0x1410, 0xa008 }, { 0x0b05, 0x1776 }, { 0x0b05, 0x1774 },
 { 0x19d2, 0xfff3 }, { 0x19d2, 0xfff2 }, { 0x1557, 0x0a80 }, { 0x05c6, 0x9001 },
 { 0x05c6, 0x9002 }, { 0x05c6, 0x9202 }, { 0x05c6, 0x9203 }, { 0x05c6, 0x9222 },
 { 0x05c6, 0x9008 }, { 0x05c6, 0x9201 }, { 0x05c6, 0x9221 }, { 0x05c6, 0x9231 },
 { 0x1f45, 0x0001 }, { 0x413c, 0x8185 }, { 0x413c, 0x8186 }, { 0x05c6, 0x9208 },
 { 0x05c6, 0x920b }, { 0x05c6, 0x9224 }, { 0x05c6, 0x9225 }, { 0x05c6, 0x9244 },
 { 0x05c6, 0x9245 }, { 0x03f0, 0x241d }, { 0x03f0, 0x251d }, { 0x05c6, 0x9214 },
 { 0x05c6, 0x9215 }, { 0x05c6, 0x9264 }, { 0x05c6, 0x9265 }, { 0x05c6, 0x9234 },
 { 0x05c6, 0x9235 }, { 0x05c6, 0x9274 }, { 0x05c6, 0x9275 }, { 0x1199, 0x9000 },
 { 0x1199, 0x9001 }, { 0x1199, 0x9002 }, { 0x1199, 0x9003 }, { 0x1199, 0x9004 },
 { 0x1199, 0x9005 }, { 0x1199, 0x9006 }, { 0x1199, 0x9007 }, { 0x1199, 0x9008 },
 { 0x1199, 0x9009 }, { 0x1199, 0x900a }, { 0x16d8, 0x8001 }, { 0x16d8, 0x8002 },
 { 0x05c6, 0x9204 }, { 0x05c6, 0x9205 }, { 0x0af0, 0x5000 }, { 0x0af0, 0x6000 },
 { 0x0af0, 0x6100 }, { 0x0af0, 0x6200 }, { 0x0af0, 0x6300 }, { 0x0af0, 0x6050 },
 { 0x0af0, 0x6150 }, { 0x0af0, 0x6250 }, { 0x0af0, 0x6350 }, { 0x0af0, 0x6500 },
 { 0x0af0, 0x6501 }, { 0x0af0, 0x6600 }, { 0x0af0, 0x6601 }, { 0x0af0, 0x6701 },
 { 0x0af0, 0x6721 }, { 0x0af0, 0x6741 }, { 0x0af0, 0x6761 }, { 0x0af0, 0x6800 },
 { 0x0af0, 0x6901 }, { 0x0af0, 0x7001 }, { 0x0af0, 0x7021 }, { 0x0af0, 0x7041 },
 { 0x0af0, 0x7061 }, { 0x0af0, 0x7100 }, { 0x0af0, 0x7201 }, { 0x0408, 0xea02 },
 { 0x0408, 0xea03 }, { 0x0408, 0xea04 }, { 0x0408, 0xea05 }, { 0x0408, 0xea06 },
 { 0x12d1, 0x1001 }, { 0x12d1, 0x1003 }, { 0x12d1, 0x1004 }, { 0x12d1, 0x1401 },
 { 0x12d1, 0x1402 }, { 0x12d1, 0x1403 }, { 0x12d1, 0x1404 }, { 0x12d1, 0x1405 },
 { 0x12d1, 0x1406 }, { 0x12d1, 0x1407 }, { 0x12d1, 0x1408 }, { 0x12d1, 0x1409 },
 { 0x12d1, 0x140a }, { 0x12d1, 0x140b }, { 0x12d1, 0x140c }, { 0x12d1, 0x140d },
 { 0x12d1, 0x140e }, { 0x12d1, 0x140f }, { 0x12d1, 0x1410 }, { 0x12d1, 0x1411 },
 { 0x12d1, 0x1412 }, { 0x12d1, 0x1413 }, { 0x12d1, 0x1414 }, { 0x12d1, 0x1415 },
 { 0x12d1, 0x1416 }, { 0x12d1, 0x1417 }, { 0x12d1, 0x1418 }, { 0x12d1, 0x1419 },
 { 0x12d1, 0x141a }, { 0x12d1, 0x141b }, { 0x12d1, 0x141c }, { 0x12d1, 0x141d },
 { 0x12d1, 0x141e }, { 0x12d1, 0x141f }, { 0x12d1, 0x1420 }, { 0x12d1, 0x1421 },
 { 0x12d1, 0x1422 }, { 0x12d1, 0x1423 }, { 0x12d1, 0x1424 }, { 0x12d1, 0x1425 },
 { 0x12d1, 0x1426 }, { 0x12d1, 0x1427 }, { 0x12d1, 0x1428 }, { 0x12d1, 0x1429 },
 { 0x12d1, 0x142a }, { 0x12d1, 0x142b }, { 0x12d1, 0x142c }, { 0x12d1, 0x142d },
 { 0x12d1, 0x142e }, { 0x12d1, 0x142f }, { 0x12d1, 0x1430 }, { 0x12d1, 0x1431 },
 { 0x12d1, 0x1432 }, { 0x12d1, 0x1433 }, { 0x12d1, 0x1434 }, { 0x12d1, 0x1435 },
 { 0x12d1, 0x1436 }, { 0x12d1, 0x1437 }, { 0x12d1, 0x1438 }, { 0x12d1, 0x1439 },
 { 0x12d1, 0x143a }, { 0x12d1, 0x143b }, { 0x12d1, 0x143c }, { 0x12d1, 0x143d },
 { 0x12d1, 0x143e }, { 0x12d1, 0x143f }, { 0x12d1, 0x1464 }, { 0x12d1, 0x1465 },
 { 0x12d1, 0x1803 }, { 0x12d1, 0x14ac }, { 0x1410, 0x1100 }, { 0x1410, 0x1110 },
 { 0x1410, 0x1120 }, { 0x1410, 0x1130 }, { 0x1410, 0x1400 }, { 0x1410, 0x1410 },
 { 0x1410, 0x1420 }, { 0x1410, 0x1430 }, { 0x1410, 0x1450 }, { 0x1410, 0x2100 },
 { 0x1410, 0x2110 }, { 0x1410, 0x2120 }, { 0x1410, 0x2130 }, { 0x1410, 0x2400 },
 { 0x1410, 0x2410 }, { 0x1410, 0x2420 }, { 0x1410, 0x4400 }, { 0x1410, 0x4100 },
 { 0x1410, 0x6002 }, { 0x1410, 0x6010 }, { 0x1410, 0x6000 }, { 0x1410, 0x7000 },
 { 0x1410, 0x8000 }, { 0x1410, 0x9000 }, { 0x1410, 0x6001 }, { 0x1410, 0x7003 },
 { 0x1410, 0x7004 }, { 0x1410, 0x7005 }, { 0x1410, 0x7006 }, { 0x1410, 0x7007 },
 { 0x1410, 0x7030 }, { 0x1410, 0x7041 }, { 0x1410, 0x7042 }, { 0x1410, 0x8001 },
 { 0x1410, 0x9001 }, { 0x1410, 0xa001 }, { 0x1410, 0xa002 }, { 0x1410, 0xa010 },
 { 0x1614, 0x0800 }, { 0x1614, 0x7002 }, { 0x1614, 0x0802 }, { 0x1614, 0x0407 },
 { 0x413c, 0x8114 }, { 0x413c, 0x8115 }, { 0x413c, 0x8116 }, { 0x413c, 0x8117 },
 { 0x413c, 0x8118 }, { 0x413c, 0x8128 }, { 0x413c, 0x8129 }, { 0x413c, 0x8133 },
 { 0x413c, 0x8134 }, { 0x413c, 0x8135 }, { 0x413c, 0x8136 }, { 0x413c, 0x8137 },
 { 0x413c, 0x8138 }, { 0x413c, 0x8180 }, { 0x413c, 0x8181 }, { 0x413c, 0x8182 },
 { 0x16d5, 0x6501 }, { 0x16d5, 0x6502 }, { 0x16d5, 0x6202 }, { 0x1726, 0x1000 },
 { 0x0eab, 0xc893 }, { 0x1a8d, 0x1002 }, { 0x1a8d, 0x1003 }, { 0x1a8d, 0x1004 },
 { 0x1a8d, 0x1005 }, { 0x1a8d, 0x1006 }, { 0x1a8d, 0x1007 }, { 0x1a8d, 0x1008 },
 { 0x1a8d, 0x1009 }, { 0x1a8d, 0x100a }, { 0x1a8d, 0x100b }, { 0x1a8d, 0x100c },
 { 0x1a8d, 0x100d }, { 0x1a8d, 0x100e }, { 0x1a8d, 0x100f }, { 0x1a8d, 0x1010 },
 { 0x1a8d, 0x1011 }, { 0x1a8d, 0x1012 }, { 0x0c88, 0x17da }, { 0x0c88, 0x180a },
 { 0x05c6, 0x6000 }, { 0x05c6, 0x6613 }, { 0x16d8, 0x6280 }, { 0x16d8, 0x6008 },
 { 0x1bc7, 0x1003 }, { 0x1bc7, 0x1004 }, { 0x19d2, 0x0001 }, { 0x19d2, 0x0002 },
 { 0x19d2, 0x0003 }, { 0x19d2, 0x0004 }, { 0x19d2, 0x0005 }, { 0x19d2, 0x0006 },
 { 0x19d2, 0x0007 }, { 0x19d2, 0x0008 }, { 0x19d2, 0x0009 }, { 0x19d2, 0x000a },
 { 0x19d2, 0x000b }, { 0x19d2, 0x000c }, { 0x19d2, 0x000d }, { 0x19d2, 0x000e },
 { 0x19d2, 0x000f }, { 0x19d2, 0x0010 }, { 0x19d2, 0x0011 }, { 0x19d2, 0x0012 },
 { 0x19d2, 0x0013 }, { 0x19d2, 0x0014 }, { 0x19d2, 0x0015 }, { 0x19d2, 0x0016 },
 { 0x19d2, 0x0017 }, { 0x19d2, 0x0018 }, { 0x19d2, 0x0019 }, { 0x19d2, 0x0020 },
 { 0x19d2, 0x0021 }, { 0x19d2, 0x0022 }, { 0x19d2, 0x0023 }, { 0x19d2, 0x0024 },
 { 0x19d2, 0x0025 }, { 0x19d2, 0x0028 }, { 0x19d2, 0x0029 }, { 0x19d2, 0x0030 },
 { 0x19d2, 0x0031 }, { 0x19d2, 0x0032 }, { 0x19d2, 0x0033 }, { 0x19d2, 0x0034 },
 { 0x19d2, 0x0037 }, { 0x19d2, 0x0038 }, { 0x19d2, 0x0039 }, { 0x19d2, 0x0040 },
 { 0x19d2, 0x0042 }, { 0x19d2, 0x0043 }, { 0x19d2, 0x0044 }, { 0x19d2, 0x0048 },
 { 0x19d2, 0x0049 }, { 0x19d2, 0x0050 }, { 0x19d2, 0x0051 }, { 0x19d2, 0x0052 },
 { 0x19d2, 0x0054 }, { 0x19d2, 0x0055 }, { 0x19d2, 0x0056 }, { 0x19d2, 0x0057 },
 { 0x19d2, 0x0058 }, { 0x19d2, 0x0059 }, { 0x19d2, 0x0061 }, { 0x19d2, 0x0062 },
 { 0x19d2, 0x0063 }, { 0x19d2, 0x0064 }, { 0x19d2, 0x0065 }, { 0x19d2, 0x0066 },
 { 0x19d2, 0x0067 }, { 0x19d2, 0x0069 }, { 0x19d2, 0x0070 }, { 0x19d2, 0x0076 },
 { 0x19d2, 0x0077 }, { 0x19d2, 0x0078 }, { 0x19d2, 0x0079 }, { 0x19d2, 0x0082 },
 { 0x19d2, 0x0083 }, { 0x19d2, 0x0086 }, { 0x19d2, 0x0087 }, { 0x19d2, 0x0104 },
 { 0x19d2, 0x0105 }, { 0x19d2, 0x0106 }, { 0x19d2, 0x0108 }, { 0x19d2, 0x0113 },
 { 0x19d2, 0x0117 }, { 0x19d2, 0x0118 }, { 0x19d2, 0x0121 }, { 0x19d2, 0x0122 },
 { 0x19d2, 0x0123 }, { 0x19d2, 0x0124 }, { 0x19d2, 0x0125 }, { 0x19d2, 0x0126 },
 { 0x19d2, 0x0128 }, { 0x19d2, 0x0142 }, { 0x19d2, 0x0143 }, { 0x19d2, 0x0144 },
 { 0x19d2, 0x0145 }, { 0x19d2, 0x0146 }, { 0x19d2, 0x0147 }, { 0x19d2, 0x0148 },
 { 0x19d2, 0x0149 }, { 0x19d2, 0x0150 }, { 0x19d2, 0x0151 }, { 0x19d2, 0x0152 },
 { 0x19d2, 0x0153 }, { 0x19d2, 0x0154 }, { 0x19d2, 0x0155 }, { 0x19d2, 0x0156 },
 { 0x19d2, 0x0157 }, { 0x19d2, 0x0158 }, { 0x19d2, 0x0159 }, { 0x19d2, 0x0160 },
 { 0x19d2, 0x0161 }, { 0x19d2, 0x0162 }, { 0x19d2, 0x1008 }, { 0x19d2, 0x1010 },
 { 0x19d2, 0x1012 }, { 0x19d2, 0x1057 }, { 0x19d2, 0x1058 }, { 0x19d2, 0x1059 },
 { 0x19d2, 0x1060 }, { 0x19d2, 0x1061 }, { 0x19d2, 0x1062 }, { 0x19d2, 0x1063 },
 { 0x19d2, 0x1064 }, { 0x19d2, 0x1065 }, { 0x19d2, 0x1066 }, { 0x19d2, 0x1067 },
 { 0x19d2, 0x1068 }, { 0x19d2, 0x1069 }, { 0x19d2, 0x1070 }, { 0x19d2, 0x1071 },
 { 0x19d2, 0x1072 }, { 0x19d2, 0x1073 }, { 0x19d2, 0x1074 }, { 0x19d2, 0x1075 },
 { 0x19d2, 0x1076 }, { 0x19d2, 0x1077 }, { 0x19d2, 0x1078 }, { 0x19d2, 0x1079 },
 { 0x19d2, 0x1080 }, { 0x19d2, 0x1081 }, { 0x19d2, 0x1082 }, { 0x19d2, 0x1083 },
 { 0x19d2, 0x1084 }, { 0x19d2, 0x1085 }, { 0x19d2, 0x1086 }, { 0x19d2, 0x1087 },
 { 0x19d2, 0x1088 }, { 0x19d2, 0x1089 }, { 0x19d2, 0x1090 }, { 0x19d2, 0x1091 },
 { 0x19d2, 0x1092 }, { 0x19d2, 0x1093 }, { 0x19d2, 0x1094 }, { 0x19d2, 0x1095 },
 { 0x19d2, 0x1096 }, { 0x19d2, 0x1097 }, { 0x19d2, 0x1098 }, { 0x19d2, 0x1099 },
 { 0x19d2, 0x1100 }, { 0x19d2, 0x1101 }, { 0x19d2, 0x1102 }, { 0x19d2, 0x1103 },
 { 0x19d2, 0x1104 }, { 0x19d2, 0x1105 }, { 0x19d2, 0x1106 }, { 0x19d2, 0x1107 },
 { 0x19d2, 0x1108 }, { 0x19d2, 0x1109 }, { 0x19d2, 0x1110 }, { 0x19d2, 0x1111 },
 { 0x19d2, 0x1112 }, { 0x19d2, 0x1113 }, { 0x19d2, 0x1114 }, { 0x19d2, 0x1115 },
 { 0x19d2, 0x1116 }, { 0x19d2, 0x1117 }, { 0x19d2, 0x1118 }, { 0x19d2, 0x1119 },
 { 0x19d2, 0x1120 }, { 0x19d2, 0x1121 }, { 0x19d2, 0x1122 }, { 0x19d2, 0x1123 },
 { 0x19d2, 0x1124 }, { 0x19d2, 0x1125 }, { 0x19d2, 0x1126 }, { 0x19d2, 0x1127 },
 { 0x19d2, 0x1128 }, { 0x19d2, 0x1129 }, { 0x19d2, 0x1130 }, { 0x19d2, 0x1131 },
 { 0x19d2, 0x1132 }, { 0x19d2, 0x1133 }, { 0x19d2, 0x1134 }, { 0x19d2, 0x1135 },
 { 0x19d2, 0x1136 }, { 0x19d2, 0x1137 }, { 0x19d2, 0x1138 }, { 0x19d2, 0x1139 },
 { 0x19d2, 0x1140 }, { 0x19d2, 0x1141 }, { 0x19d2, 0x1142 }, { 0x19d2, 0x1143 },
 { 0x19d2, 0x1144 }, { 0x19d2, 0x1145 }, { 0x19d2, 0x1146 }, { 0x19d2, 0x1147 },
 { 0x19d2, 0x1148 }, { 0x19d2, 0x1149 }, { 0x19d2, 0x1150 }, { 0x19d2, 0x1151 },
 { 0x19d2, 0x1152 }, { 0x19d2, 0x1153 }, { 0x19d2, 0x1154 }, { 0x19d2, 0x1155 },
 { 0x19d2, 0x1156 }, { 0x19d2, 0x1157 }, { 0x19d2, 0x1158 }, { 0x19d2, 0x1159 },
 { 0x19d2, 0x1160 }, { 0x19d2, 0x1161 }, { 0x19d2, 0x1162 }, { 0x19d2, 0x1163 },
 { 0x19d2, 0x1164 }, { 0x19d2, 0x1165 }, { 0x19d2, 0x1166 }, { 0x19d2, 0x1167 },
 { 0x19d2, 0x1168 }, { 0x19d2, 0x1169 }, { 0x19d2, 0x1170 }, { 0x19d2, 0x1244 },
 { 0x19d2, 0x1245 }, { 0x19d2, 0x1246 }, { 0x19d2, 0x1247 }, { 0x19d2, 0x1248 },
 { 0x19d2, 0x1249 }, { 0x19d2, 0x1250 }, { 0x19d2, 0x1251 }, { 0x19d2, 0x1252 },
 { 0x19d2, 0x1253 }, { 0x19d2, 0x1254 }, { 0x19d2, 0x1255 }, { 0x19d2, 0x1256 },
 { 0x19d2, 0x1257 }, { 0x19d2, 0x1258 }, { 0x19d2, 0x1259 }, { 0x19d2, 0x1260 },
 { 0x19d2, 0x1261 }, { 0x19d2, 0x1262 }, { 0x19d2, 0x1263 }, { 0x19d2, 0x1264 },
 { 0x19d2, 0x1265 }, { 0x19d2, 0x1266 }, { 0x19d2, 0x1267 }, { 0x19d2, 0x1268 },
 { 0x19d2, 0x1269 }, { 0x19d2, 0x1270 }, { 0x19d2, 0x1271 }, { 0x19d2, 0x1272 },
 { 0x19d2, 0x1273 }, { 0x19d2, 0x1274 }, { 0x19d2, 0x1275 }, { 0x19d2, 0x1276 },
 { 0x19d2, 0x1277 }, { 0x19d2, 0x1278 }, { 0x19d2, 0x1279 }, { 0x19d2, 0x1280 },
 { 0x19d2, 0x1281 }, { 0x19d2, 0x1282 }, { 0x19d2, 0x1283 }, { 0x19d2, 0x1284 },
 { 0x19d2, 0x1285 }, { 0x19d2, 0x1286 }, { 0x19d2, 0x1287 }, { 0x19d2, 0x1288 },
 { 0x19d2, 0x1289 }, { 0x19d2, 0x1290 }, { 0x19d2, 0x1291 }, { 0x19d2, 0x1292 },
 { 0x19d2, 0x1293 }, { 0x19d2, 0x1294 }, { 0x19d2, 0x1295 }, { 0x19d2, 0x1296 },
 { 0x19d2, 0x1297 }, { 0x19d2, 0x1298 }, { 0x19d2, 0x1299 }, { 0x19d2, 0x1300 },
 { 0x19d2, 0x0014 }, { 0x19d2, 0x0027 }, { 0x19d2, 0x0059 }, { 0x19d2, 0x0060 },
 { 0x19d2, 0x0070 }, { 0x19d2, 0x0073 }, { 0x19d2, 0x0130 }, { 0x19d2, 0x0141 },
 { 0x19d2, 0x2002 }, { 0x19d2, 0x2003 }, { 0x19d2, 0xfffe }, { 0x19d2, 0xfff1 },
 { 0x19d2, 0xfff5 }, { 0x19d2, 0xffff }, { 0x1d6b, 0x0002 }, { 0x04a5, 0x4068 },
 { 0x1186, 0x3e04 }, { 0x1e0e, 0xce16 }, { 0x1e0e, 0xce1e }, { 0x1da5, 0x4512 },
 { 0x1da5, 0x4523 }, { 0x1da5, 0x4515 }, { 0x1da5, 0x4518 }, { 0x1da5, 0x4519 },
 { 0x0930, 0x0d45 }, { 0x0930, 0x1302 }, { 0x1e0e, 0x9000 }, { 0x1e0e, 0x9200 },
 { 0x1bbb, 0x0000 }, { 0x1011, 0x3198 }, { 0x20b9, 0x1682 }, { 0x1c9e, 0x9603 },
 { 0x201e, 0x2009 }, { 0x1266, 0x1002 }, { 0x1266, 0x1003 }, { 0x1266, 0x1004 },
 { 0x1266, 0x1005 }, { 0x1266, 0x1006 }, { 0x1266, 0x1007 }, { 0x1266, 0x1008 },
 { 0x1266, 0x1009 }, { 0x1266, 0x100a }, { 0x1266, 0x100b }, { 0x1266, 0x100c },
 { 0x1266, 0x100d }, { 0x1266, 0x100e }, { 0x1266, 0x100f }, { 0x1266, 0x1011 },
 { 0x1266, 0x1012 }, { 0x0681, 0x0047 }, { 0x0b3c, 0xc000 }, { 0x211f, 0x6801 },
 { 0x1ee8, 0x000b }
};

typedef struct _devInfo {
    int type;           /* Device type, a DEV_TYPE_* value */
    int id;             /* Device id */
    int Vid;            /* Vendor ID */
    int Pid;            /* Product ID */
    char *serial;       /* Serial number string */
    int bus_num;        /* Number of bus device is on */
    int dev_num;        /* Number of device in bus */
    char *name;         /* Friendly name */
    char *fullName;     /* Full name, including manufacturer, etc */
    char *sysName;      /* Name in /sys of devices info directory */
    int VM;             /* Domid of VM attached to, or a DEV_VM_* value */
    int classCount;     /* Number of classes device specifies */
    int *classes;       /* Classes device specifies */
    int ifaceCount;     /* Number of interfaces we currently have */
    struct _devInfo *next;
} DevInfo;

static DevInfo *devices = NULL;

#define DEV_VM_NONE           -1 /* Device currently not attached to any VM */
#define DEV_VM_DOM0           0  /* Device assigned to dom0 */

#define DEV_TYPE_NORMAL       0  /* Normal devices */
#define DEV_TYPE_HIDU         1  /* HiD Unknown, don't yet know whether includes a keyboard, cannot be reassigned */
#define DEV_TYPE_HIDJ         2  /* HiD Joystick, all classes HiD, does not include a keyboard */
#define DEV_TYPE_HIDK         3  /* HiD Keyboard, includes at least one Keyboard, cannot be reassigned */
#define DEV_TYPE_HIDP         4  /* HiD Partial, not all classes HiD, does not include a keyboard */
#define DEV_TYPE_HUB          5  /* Hubs, cannot be assigned */
#define DEV_TYPE_EXT_CD       6  /* External CD drives */
#define DEV_TYPE_STORAGE      7  /* A storage device which may be a CD drive */ 
#define DEV_TYPE_CDC          8  /* A CDC device */
#define DEV_TYPE_MAX          9  /* Unassignable value for range checking */

static const char *dev_type_names[DEV_TYPE_MAX] = {
    "Device", "HiD", "Non-keyboard", "Keyboard", "Partial HiD", "Hub", "CD", "Storage"
};

/* GUI device states */
#define DEV_STATE_ERROR       -1 /* Cannot find device */
#define DEV_STATE_UNUSED      0  /* Device not in use by any VM */
#define DEV_STATE_ASSIGNED    1  /* Assigned to another VM which is off */
#define DEV_STATE_IN_USE      2  /* Assigned to another VM which is running */
#define DEV_STATE_BLOCKED     3  /* Blocked by policy for this VM */
#define DEV_STATE_THIS        4  /* In use by this VM */
#define DEV_STATE_THIS_ALWAYS 5  /* In use by this VM and flagged "always" */
#define DEV_STATE_ALWAYS_ONLY 6  /* Flagged as "always" assigned to this VM, but not currently in use */
#define DEV_STATE_PLATFORM    7  /* Special platform device, listed purely for information */
#define DEV_STATE_HID_DOM0    8  /* HiD device assigned to dom0 */
#define DEV_STATE_HID_ALWAYS  9  /* HiD device currently assigned to dom0, but always assigned to another VM */
#define DEV_STATE_CD_DOM0     10 /* External CD drive assigned to dom0 */
#define DEV_STATE_CD_ALWAYS   11 /* External CD drive currently assigned to dom0, but always assigned to another VM */

#if 0
#define FIRST_RUN_FILE  "/var/tmp/ctxusb_daemon"  /* File we touch to indicate we've run */
#endif

#define USB_MASS_STORAGE      0x0806  /* Mass storage class and subclass */



/**** Misc utilities ****/


/* Check if device can be suspendd
 * param    Vid              Vendor id
 * param    Pid              Product id
 * return                    1 if can be suspended, 0 otherwise
 */
static int device_can_be_suspended(int Vid, int Pid)
{
  int i, entries = sizeof(usb_network_adapters)/sizeof(struct usb_vendor_product);
  for (i = 0; i < entries; i++) {
    if (usb_network_adapters[i].Vid == Vid && usb_network_adapters[i].Pid == Pid) {
      LogInfo("Device v0x%04x, p0x%04x can not be suspended\n", Vid, Pid);
      return 0;
    }
  }
  return 1;
}

/* Generate a device ID
 * param    bus_num              number of bus device is on
 * param    dev_num              device number within bus
 * return                        device id
 */
int makeDeviceId(int bus_num, int dev_num)
{
    return ((bus_num & 0xFF) << 8) | (dev_num & 0xFF);
}

void makeBusDevPair(int devid, int *bus_num, int *dev_num)
{
    *bus_num = devid >> 8;
    *dev_num = devid & 0xFF;
}


/* Find the device with the given id
 * param    id                   id of device to find
 * return                        specified device, NULL if not found
 */
static DevInfo *findDevice(int id)
{
    DevInfo *p;

    for(p = devices; p != NULL; p = p->next)
        if(p->id == id)
            return p;

    /* No match */
    return NULL;
}


/* Clarify whether the given HIDU device is actually a HIDJ or a HIDP
 * param    device                device to clarify
 */
static void clarifyHid(DevInfo *device)
{
    int i;  /* Class counter */

    assert(device != NULL);
    assert(device->type == DEV_TYPE_HIDU);

    device->type = DEV_TYPE_HIDJ;  /* Start by assuming all classes are HiD */
        
    for(i = 0; i < device->classCount; ++i)
    {
        if(((device->classes[i] >> 8) & 0xFF) != USB_CLASS_HID)
        {
            /* Not all interfaces are HiD */
            device->type = DEV_TYPE_HIDP;
            return;
        }
    }
}


/* Report whether a HiD is a keyboard
 * param    device               device we're reporting about
 * param    hasKeyboard          0 if device contains a keyboard, >0 for no keyboard, <0 on detection error
 * return                        0 if device is considered to contain a keyboard, non-0 otherwise
 */
static int reportHidKeyboard(DevInfo *device, int hasKeyboard)
{
    assert(device != NULL);

    if(hasKeyboard > 0)  /* Device does not include a keyboard */
        return hasKeyboard;

    if(hasKeyboard == 0)
        LogInfo("Device %d includes a keyboard, preventing assignment", device->id);
    else
        LogInfo("Can't tell if device %d includes a keyboard, assume it does, preventing assignment", device->id);

    device->type = DEV_TYPE_HIDK;
    device->VM = DEV_VM_DOM0;
    return 0;
}


/* Check a device's policy
 * param    device               device to check
 * param    dom_id               id of domain to check against
 * param    dom_uuid             uuid of domain to check against
 * return                        0 => success, non-0 => blocked
 */
static int checkDevicePolicy(DevInfo *device, int dom_id, const char *dom_uuid)
{
    assert(device != NULL);
    assert(dom_uuid != NULL);

    if(dom_id == DEV_VM_DOM0 ||
       consultPolicy(dom_uuid, device->Vid, device->Pid, device->classCount, device->classes) != 0)
        return 0;

    LogInfo("Device \"%s\" blocked to domain %d by policy", device->name, dom_id);
    remote_report_rejected(device->name, "policy");
    return 1;
}

void clearStickyIfPolicyBlocked( int devid, const char *dom_uuid )
{
    char *always_uuid = NULL;
    DevInfo *dev = findDevice(devid);

    if (!dev) {
        return;
    }
    if ( consultPolicy(dom_uuid, dev->Vid, dev->Pid, dev->classCount, dev->classes) != 0 ) {
        // allowed
        return;
    }
    // rejected -> clear sticky maybe
    always_uuid = alwaysGet( dev->Vid, dev->Pid, dev->serial );
    if (always_uuid && !strcmp(always_uuid, dom_uuid)) {
        LogInfo("Clearing sticky bit on device %s due to domain access being blocked with policy", dev->name);
        setSticky( devid, 0 );
    }
}

/**** Functions to perform (de)connections between devices and VMs ****/

/* Move the given device to the specified VM
 * param    device               device to plug in
 * param    dom_uuid             UUID of VM to plug device into
 */
static void moveDeviceToVM(DevInfo *device, const char *dom_uuid)
{
    int dom_id;  /* ID of domain to plug into */

    assert(device != NULL);
    assert(dom_uuid != NULL);

    LogInfo("Moving device %d to domain %s", device->id, dom_uuid);

    dom_id = VMuuid2id(dom_uuid);

    if(dom_id < 0)
    {
        LogInfo("Cannot plug device \"%s\" into VM %s, not running", device->name, dom_uuid);
        return;
    }

    if(dom_id == getUIVMid())
        dom_id = DEV_VM_DOM0;

    if((device->type == DEV_TYPE_HIDK || device->type == DEV_TYPE_HUB) && dom_id != DEV_VM_DOM0)
    {
        LogInfo("Cannot plug device \"%s\" into VM, platform only device", device->name);
        return;
    }

    if(device->type == DEV_TYPE_HIDU && dom_id != DEV_VM_DOM0)
    {
        LogInfo("Cannot plug device \"%s\" into VM, still determining nature", device->name);
        return;
    }

    if(checkDevicePolicy(device, dom_id, dom_uuid) != 0)
        return;

    /* Unplug from current VM, if any */
    if(device->VM != DEV_VM_DOM0 && device->VM != DEV_VM_NONE)
    {
        LogInfo("Unplugging device \"%s\" from domain %d", device->name, device->VM);
        remote_unplug_device(device->VM, device->bus_num, device->dev_num);
    }

    /* Reset device in case previous VM left it in a bad state */
    resetDevice(device->sysName, device->bus_num, device->dev_num);

    /* HiD devices will be rechecked for keyboards after reset and an rpc will be sent to the target VM
     * as part of the revert. For non-HiD devices we miss all of that out and do the rpc here.
     */

    device->VM = dom_id;

    if(device->VM != DEV_VM_DOM0 && device->type != DEV_TYPE_HIDU && device->type != DEV_TYPE_HIDP &&
       device->type != DEV_TYPE_HIDJ && device->type != DEV_TYPE_HIDK)
    {
        LogInfo("Giving device \"%s\" to domain %d\n", device->name, dom_id);
        resumeDevice(device->sysName);
        if(remote_plug_device(dom_id, device->bus_num, device->dev_num))
            LogError("Failed to plug device \"%s\" into domain %d", device->name, dom_id);
    }

    if(device->VM == DEV_VM_DOM0 && device->type != DEV_TYPE_HIDU && 
       device->type != DEV_TYPE_HIDP && device->type != DEV_TYPE_HIDJ &&
       device->type != DEV_TYPE_HIDK && device->type != DEV_TYPE_STORAGE &&
       device->type != DEV_TYPE_HUB && device->type != DEV_TYPE_CDC &&
       device->type != DEV_TYPE_EXT_CD)
      {
	if (device_can_be_suspended(device->Vid, device->Pid)) {
	  LogInfo("Device not useful to Dom0, suspending it.");
	  suspendDevice(device->sysName);
	}
      }

    remote_report_dev_change(device->id);
}


/* Borrow specified device from current VM, if any
 * param    device               device to borrow
 */
static void borrowDeviceFromVM(DevInfo *device)
{
    assert(device != NULL);

    if(device->VM == DEV_VM_DOM0 || device->VM == DEV_VM_NONE)
        return;

    LogInfo("Borrowing device \"%s\" from domain %d", device->name, device->VM);
    remote_unplug_device(device->VM, device->bus_num, device->dev_num);
}

/* Route a new device to the relevant domain
 * param    device               device to route
 * param    dom_id               id of domain to route device to, DEV_VM_NONE => use standard routing
 */
static int findDeviceRoute(DevInfo *device, int dom_id)
{
    const char *uuid;  /* uuid of target VM */

    assert(device != NULL);

    uuid = NULL;

    /* Work out where to send device */
    if(device->type == DEV_TYPE_HIDK || device->type == DEV_TYPE_HUB)
    {
        dom_id = DEV_VM_DOM0;
        uuid = DOM0_UUID;
    }
    else if(dom_id == DEV_VM_NONE)
    {
        updateVMs(-1);

        uuid = alwaysGet(device->Vid, device->Pid, device->serial);

        if(uuid != NULL)
        {
            dom_id = VMuuid2id(uuid);

            if(dom_id < 0)
            {
                LogInfo("Cannot plug device \"%s\" into VM %s, not running. Rerouting to dom0", device->name, uuid);
                dom_id = DEV_VM_DOM0;
                uuid = DOM0_UUID;
            }
        }
        else
        {
            /* Device has an no always flag */
            if(device->type == DEV_TYPE_HIDJ)
            {
                /* Device is a HiD, assign to dom0 */
                dom_id = DEV_VM_DOM0;
                uuid = DOM0_UUID;
            }
            else
            {
                /* Device is not a HiD, assign to VM in focus */
                if(xcdbus_input_get_focus_domid(rpc_xcbus(), &dom_id) == 0)
                {
                    LogError("Could not query focused domain, assuming dom0");
                    dom_id = DEV_VM_DOM0;
                    uuid = DOM0_UUID;
                }
                if (is_usb_enabled(dom_id))
                {
                    LogInfo("SSSS domid=%d is_usb_enabled=true", dom_id);
                }
                else
                {
                    LogInfo("SSSS domid=%d is_usb_enabled=false", dom_id);
                    dom_id = DEV_VM_DOM0;
                }
            }
        }
    }

    /* UIVM => dom0 */
    if(dom_id == getUIVMid())
    {
        dom_id = DEV_VM_DOM0;
        uuid = DOM0_UUID;
    }

    /* Find target domain uuid, if we haven't already */
    if(uuid == NULL)
    {
        uuid = VMid2uuid(dom_id);
        
        if(uuid == NULL)
        {
            LogError("Want to assign device \"%s\" to VM %d, but can't find its UUID",
                     device->name, dom_id);
            dom_id = DEV_VM_DOM0;
            uuid = DOM0_UUID;
        }
    }

    if(checkDevicePolicy(device, dom_id, uuid) != 0)
        dom_id = DEV_VM_DOM0;

    return dom_id;
}

static void routeDevice(DevInfo *device, int dom_id)
{
    int routed_dom_id = findDeviceRoute(device,dom_id);

    if(routed_dom_id == DEV_VM_DOM0)
    {
      bindAllToDom0(device->sysName);
      if(device->type != DEV_TYPE_HIDU && device->type != DEV_TYPE_HIDP &&
	 device->type != DEV_TYPE_HIDJ && device->type != DEV_TYPE_HIDK &&
	 device->type != DEV_TYPE_STORAGE && device->type != DEV_TYPE_HUB &&
	 device->type != DEV_TYPE_CDC && device->type != DEV_TYPE_EXT_CD)
	{
	  if (device_can_be_suspended(device->Vid, device->Pid)) {
	    LogInfo("Device not useful to Dom0, suspending it.");
	    suspendDevice(device->sysName);
	  }
	}
    }
    else
    {
        char *uuid = VMid2uuid(routed_dom_id);
        if (uuid)
            moveDeviceToVM(device, uuid);
    }

    remote_report_dev_change(device->id);
}



/**** Functions to access the device store ****/

/* Delete the given device info storage
 * param    devInfo              device info to delete
 */
static void deleteDevice(DevInfo *devInfo)
{
    if(devInfo == NULL)
        return;

    if(devInfo->serial != NULL)
        free(devInfo->serial);

    if(devInfo->name != NULL)
        free(devInfo->name);

    if(devInfo->sysName != NULL)
        free(devInfo->sysName);

    if(devInfo->classes != NULL)
        free(devInfo->classes);

    free(devInfo);
}
    

/* Purge any device assignments to the specified VM (which has shut down)
 * param    dom_id               id of domain to purge devices from
 */
void purgeDeviceAssignments(int dom_id)
{
    DevInfo *p;

    assert(dom_id > 0);

    LogInfo("Spotted VM %d stopping", dom_id);

    for(p = devices; p != NULL; p = p->next)
    {
        if(p->VM == dom_id)
        {
            LogInfo("Removing device \"%s\" from VM %d", p->name, dom_id);
            p->VM = DEV_VM_DOM0;  /* Prevent attempts to remote unplug */
            moveDeviceToVM(p, DOM0_UUID);
        }
    }
}


/* Add a device to our list
 * param    Vid                  vendor id of device
 * param    Pid                  product id of device
 * param    serial               serial number of device
 * param    bus_num              bus number of device
 * param    dev_num              device number (within bus)
 * param    name                 friendly device name
 * param    fullName             full human readable name, including manufacturer, etc
 * param    sysName              name of device's /sys info directory
 * param    classBuf             classes device specifies
 * return                        id of created device
 */
int addDevice(int Vid, int Pid, const char *serial, int bus_num, int dev_num, const char *name,
              const char *fullName, const char *sysName, IBuffer *classBuf)
{
    DevInfo *p;     /* New device */
    int i;          /* Class counter */
    int isHiD;      /* Is device being added a HiD (any classes are HiD) */
    int isHub;      /* Is device being added a hub (all classes are hub) */
    int isCdc;      /* Is device being added a CDC (any classes are CDC) */
    int isStorage;  /* Is device being added mass storage (all classes are mass storage) */
    int hasNoHiDIf;

    assert(serial != NULL);
    assert(name != NULL);
    assert(sysName != NULL);
    assert(classBuf != NULL);

    /* Setup new device info */
    p = malloc(sizeof(DevInfo));
    if(p == NULL)
    {
        LogError("Cannot allocate array for DevInfo");
        exit(2);
    }

    p->id = makeDeviceId(bus_num, dev_num);
    p->Vid = Vid;
    p->Pid = Pid;
    p->serial = strcopy(serial);
    p->bus_num = bus_num;
    p->dev_num = dev_num;
    p->name = strcopy(name);
    p->fullName = strcopy(fullName);
    p->sysName = strcopy(sysName);
    p->VM = DEV_VM_NONE;
    p->ifaceCount = 0;
    p->next = NULL;

    /* Transfer class list to device info */
    p->classCount = classBuf->len;
    p->classes = malloc(sizeof(int) * classBuf->len);
    if(p->classes == NULL)
    {
        LogError("Could not allocate class buffer size %d", sizeof(int) * classBuf->len);
        exit(2);
    }

    isHiD = 0;
    isCdc = 0;
    isHub = 1;
    isStorage = 1;
    hasNoHiDIf = 0;
    for(i = 0; i < classBuf->len; ++i)
    {
        p->classes[i] = classBuf->data[i];
        if(((p->classes[i] >> 8) & 0xFF) != USB_CLASS_HUB)
            isHub = 0;

        if(((p->classes[i] >> 8) & 0xFF) == USB_CLASS_HID)
            isHiD = 1;
        else
            hasNoHiDIf = 1;

        if(((p->classes[i] >> 8) & 0xFF) == USB_CLASS_COMM ||
	   ((p->classes[i] >> 8) & 0xFF) == 0x0a /* USB_CLASS_CDC_DATA */)
            isCdc = 1;

        if(p->classes[i] != USB_MASS_STORAGE)
            isStorage = 0;
    }

    /* do not treat it as HiD if it's an Apple device (vendorId=0x05ac) and has at least a non-HiD interface, see XC-3895 */
    if (isHiD && p->Vid == 0x05ac && hasNoHiDIf) {
        LogInfo("Device %d has a HiD interface but it is Apple hybrid. Considering it non-HiD", p->id);
        isHiD = 0;
    }

    if(isHub != 0)
    {
        LogInfo("Device %d is a hub", p->id);
        p->type = DEV_TYPE_HUB;
        p->VM = DEV_VM_DOM0;
    }
    else if(isHiD != 0)
    {
        LogInfo("Device %d is a HiD", p->id);
        p->type = DEV_TYPE_HIDU;
    }
    else if(isCdc != 0)
    {
        LogInfo("Device %d is a Cdc", p->id);
        p->type = DEV_TYPE_CDC;
    }
    else if(isStorage != 0)
    {
        LogInfo("Device %d is mass storage", p->id);
        p->type = DEV_TYPE_STORAGE;
    }
    else
    {
        p->type = DEV_TYPE_NORMAL;
    }

    p->next = devices;
    devices = p;
    return p->id;
}


/* Unplug and remove the specified device from our list
 * param    sysName              system name of device to remove
 */
void removeDevice(const char *sysName)
{
    DevInfo *p, **q;

    assert(sysName != NULL);

    q = &devices;
    p = devices;

    while(p != NULL)
    {
        if(strcmp(p->sysName, sysName) == 0)
        {
            /* Device found */
            if(p->VM != DEV_VM_NONE && p->VM != DEV_VM_DOM0)
            {
                LogInfo("Unplugging device \"%s\" from domain %d", p->name, p->VM);
                remote_unplug_device(p->VM, p->bus_num, p->dev_num);
            }

            LogInfo("Removing device %s", sysName);
            *q = p->next;
            p->next = NULL;
            remote_report_dev_change(p->id);
            deleteDevice(p);
            return;
        }

        q = &p->next;
        p = p->next;
    }

    LogInfo("Cannot remove device %s - not found", sysName);
}


/* Dump out our device information, for debugging only
 * param    buffer               buffer to write state into
 */
void dumpDevice(Buffer *buffer)
{
    DevInfo *p;
    int i;

    assert(buffer != NULL);

    catToBuffer(buffer, "Devices:\n");

    for(p = devices; p != NULL; p = p->next)
    {
        if(p->type == DEV_TYPE_HUB)
        {
            catToBuffer(buffer, "  %s %d (%04X), %03d/%03d [%s] %04X:%04X\n",
                        dev_type_names[DEV_TYPE_HUB],
                        p->id, p->id, p->bus_num, p->dev_num, p->sysName, p->Vid, p->Pid);
        }
        else
        {
            catToBuffer(buffer, "  %s %d (%04X), %03d/%03d [%s] %04X:%04X, \"%s\"\n",
                        dev_type_names[p->type], p->id, p->id, p->bus_num, p->dev_num, p->sysName,
                        p->Vid, p->Pid, p->serial);
            catToBuffer(buffer, "    \"%s\" (%s)\n", p->name, p->fullName);

            if(p->VM == DEV_VM_NONE)
                catToBuffer(buffer, "    Not assigned\n");
            else
                catToBuffer(buffer, "    Assigned to domid %d\n", p->VM);

            if(p->type == DEV_TYPE_HIDU || p->type == DEV_TYPE_HIDP ||
               p->type == DEV_TYPE_HIDJ || p->type == DEV_TYPE_HIDK)
            {
                catToBuffer(buffer, "    %d interface%s, %d class%s:",
                            p->ifaceCount, (p->ifaceCount == 1) ? "" : "s",
                            p->classCount, (p->classCount == 1) ? "" : "es");
            }
            else
            {
                catToBuffer(buffer, "    %d class%s:",
                            p->classCount, (p->classCount == 1) ? "" : "es");
            }

            for(i = 0; i < p->classCount; ++i)
                catToBuffer(buffer, " %04X", p->classes[i]);

            catToBuffer(buffer, "\n");
        }
    }
}



/**** Functions used at daemon start of day when determining state of devices ****/

/* Claim the given device on behalf of the specified VM
 * param    dev_id               id of device to claim
 * param    dom_id               id of domain to claim device
 */
void claimDevice(int dev_id, int dom_id)
{
    DevInfo *p;

    p = findDevice(dev_id);

    if(p == NULL)
    {
        LogInfo("Cannot claim device %d, not found", dev_id);
        return;
    }

    if((p->type == DEV_TYPE_HIDK || p->type == DEV_TYPE_HUB) && dom_id != DEV_VM_DOM0)
    {
        LogInfo("Cannot claim device %d, platform only device", dev_id);
        return;
    }

    p->VM = dom_id;
}


/* Determine whether all our devices are external CD drives
 */
void checkAllOptical(void)
{
    DevInfo *p;

    for(p = devices; p != NULL; p = p->next)
    {
        if(p->type == DEV_TYPE_STORAGE && p->VM == DEV_VM_DOM0)
        {
            /* Not yet determined and can tell */

            if(isDeviceOptical(p->sysName) == 0)  /* Device is optical */
                p->type = DEV_TYPE_EXT_CD;
            else  /* Device is not optical */
                p->type = DEV_TYPE_NORMAL;
        }
    }
}


/* Determine whether all our devices are keyboards
 * Should only be called at daemon start of day when determining state of devices
 */
void checkAllKeyboard(void)
{
    DevInfo *p;
    int r;

    for(p = devices; p != NULL; p = p->next)
    {
        if(p->type == DEV_TYPE_HIDU && p->VM == DEV_VM_DOM0)
        {
            /* Don't worry about devices assigned to VMs, they can't be keyboards */
            r = checkHidKeyboard(p->sysName);
            reportHidKeyboard(p, r);
        }

        if(p->type == DEV_TYPE_HIDU)
            clarifyHid(p);
    }
}



/**** Functions used to collect information about newly plugged in devices ****/

/* Determine whether the specified device is an external CD drive
 * param    sysName               system name of device to check
 * param    host                  SCSI host number of device
 */
void checkDeviceOptical(const char *sysName, int host)
{
    DevInfo *p;

    assert(sysName != NULL);

    for(p = devices; p != NULL; p = p->next)
    {
        if(strcmp(p->sysName, sysName) == 0)
        {
            if(p->type != DEV_TYPE_STORAGE)  /* Already determined */
                return;

            if(isHostOptical(host, sysName) == 0)  /* Device is optical */
                p->type = DEV_TYPE_EXT_CD;
            else  /* Device is not optical */
                p->type = DEV_TYPE_NORMAL;

            remote_report_dev_change(p->id);
            return;
        }
    }

    LogInfo("Cannot check if device is optical %s, not found", sysName);
}            


/* Finish handling a new interface in a HiD
 * param    device               device containing new keyboard
 */
static void completeNewHidInterface(DevInfo *device)
{
    assert(device != NULL);

    ++device->ifaceCount;

    if(device->ifaceCount < getDeviceInterfaceCount(device->sysName))  /* More interfaces to come */
        return;

    /* We've had all the interfaces for this device */
    if(device->type == DEV_TYPE_HIDU)
    {
        /* Device is either a HiDJ or a HiDP, determine which */
        clarifyHid(device);
        LogInfo("Device %d does not include a keyboard, allowing assignment", device->id);
    }

    /* Send device to relevant device */
    routeDevice(device, device->VM);
}


/* Process a new hidraw node
 * param    hidName              sys name of hidraw node
 * param    devName              sys name of containing device
 */
void processNewHidraw(const char *hidName, const char *devName)
{
    DevInfo *p;
    int r;

    assert(devName != NULL);

    for(p = devices; p != NULL; p = p->next)
    {
        if(strcmp(p->sysName, devName) == 0)
        {
            /* Containing device found */
            if(p->type == DEV_TYPE_HIDU)
            {
                r = checkHidrawKeyboard(hidName, devName);
                reportHidKeyboard(p, r);
            }

            completeNewHidInterface(p);
            return;
        }
    }

    LogInfo("Cannot process hidraw, device %s not found", devName);
}


/* Process a new device interface
 * param    sysDev               system name of device containing interface
 * param    sysIface             system name of interface
 */
void processNewInterface(const char *sysDev, const char *sysIface)
{
    DevInfo *p;
    int r;
    char buf[32];
    
    assert(sysDev != NULL);
    assert(sysIface != NULL);

    for(p = devices; p != NULL; p = p->next)
    {
        LogInfo("p->sysName: %s\n", p->sysName);
	if (strncmp(p->sysName, "usb", 3) == 0) {
	  /* Special case for root hub */
	  sprintf(buf, "%d-0", atoi(p->sysName+3));
	} else {
	  sprintf(buf, "%s", p->sysName);
	}
	
        if(strcmp(buf, sysDev) == 0)
        {
            /* Containing device found */
            if(p->type != DEV_TYPE_HIDU && p->type != DEV_TYPE_HIDK)
            {
                /* This is a non-HiD, either newly plugged in, reset or reconfigured */
                if(p->VM == DEV_VM_DOM0)
                    bindToDom0(sysIface);

                return;
            }

            /* The containing device is a HiD, check whether this interface is */
            r = isInterfaceHid(sysIface);

	 /* This is a HiD interface, wait for the hidraw node, and see if it takes */
            if(!r  && !bindToUsbhid(sysIface))
                { /*it is hid, and hid raw took*/
		  /*udevmonthingy will process hidraw interfaces when they appear*/
                    bindToDom0(sysIface);
                    return;
                }

            if(r < 0)  /* Error, assume it's a keyboard */
                reportHidKeyboard(p, r);
                if(p->VM == DEV_VM_DOM0)
                    bindToDom0(sysIface);
            completeNewHidInterface(p);
            return;
        }
    }

    LogInfo("Cannot process interface %s, device not found", sysIface);
}


/* Process a removed device interface
 * param    sysDev               system name of device containing interface
 */
void processRemovedInterface(const char *sysDev)
{
    DevInfo *p;
    int r;
    
    assert(sysDev != NULL);

    for(p = devices; p != NULL; p = p->next)
    {
        if(strcmp(p->sysName, sysDev) == 0)
        {
            /* Containing device found */
            if(p->type != DEV_TYPE_HIDU && p->type != DEV_TYPE_HIDP &&
               p->type != DEV_TYPE_HIDJ && p->type != DEV_TYPE_HIDK)  /* Not a HiD, nothing to do */
                return;

            --p->ifaceCount;

            if(p->ifaceCount > 0)  /* Wait for remaining interfaces to be removed */
                return;

            p->ifaceCount = 0;

            borrowDeviceFromVM(p);
            p->type = DEV_TYPE_HIDU;

            /* Wait for new interfaces to appear */
            return;
        }
    }

    LogInfo("Cannot process interface, device %s not found", sysDev);
}



/**** Functions to query information about devices ****/

/* Provide a list of the devices currently plugged into the system
 * param    buffer               integer buffer to put device list into
 */
void getDeviceList(IBuffer *buffer)
{
    DevInfo *p;

    assert(buffer != NULL);

    clearIBuffer(buffer);

    for(p = devices; p != NULL; p = p->next)
        if(p->type != DEV_TYPE_HUB)
            appendIBuffer(buffer, p->id);
}


/* Provide information about the specified device
 * param    dev_id               id of device to provide information for
 * param    vm_uuid              UUID of VM to view device from
 * param    name                 buffer to store device name in
 * param    fullName             buffer to store device full name in
 * param    vm_assigned          buffer to store UUID of VM device is assigned to (if any)
 * return                        device GUI state
 */
int getDeviceInfo(int dev_id, const char *vm_uuid, Buffer *name, Buffer *fullName, Buffer *vm_assigned)
{
    DevInfo *p;  /* device */
    const char *assigned_uuid;  /* uuid of VM device is currently attached to */
    const char *always_uuid;    /* uuid of VM device always assigned to */

    assert(vm_uuid != NULL);
    assert(name != NULL);
    assert(fullName != NULL);
    assert(vm_assigned != NULL);

    clearBuffer(name);
    clearBuffer(vm_assigned);

    p = findDevice(dev_id);

    if(p == NULL || p->type == DEV_TYPE_HUB)
    {
        /* No such device, or device is a hub */
        return DEV_STATE_ERROR;
    }

    setBuffer(name, p->name);
    setBuffer(fullName, p->fullName);

    /* Check for platform only devices */
    if(p->type == DEV_TYPE_HIDK || p->type == DEV_TYPE_HIDU)
        return DEV_STATE_PLATFORM;

    /* Check for blocked devices */
    if(consultPolicy(vm_uuid, p->Vid, p->Pid, p->classCount, p->classes) == 0)
        return DEV_STATE_BLOCKED;

    always_uuid = alwaysGet(p->Vid, p->Pid, p->serial);

    if(p->VM == DEV_VM_NONE || p->VM == DEV_VM_DOM0)
    {
        /* Device is assigned to dom0 (or not currently in use) */
        if(always_uuid == NULL)
        {
            /* No always flag, so fully available */
            if(p->type == DEV_TYPE_EXT_CD)
                return DEV_STATE_CD_DOM0;

            if(p->type == DEV_TYPE_HIDJ)
                return DEV_STATE_HID_DOM0;

            return DEV_STATE_UNUSED;
        }

        setBuffer(vm_assigned, always_uuid);

        if(strcmp(always_uuid, vm_uuid) == 0)  /* Always assigned to given VM */
            return DEV_STATE_ALWAYS_ONLY;

        /* Always assigned to some other VM */
        if(p->type == DEV_TYPE_EXT_CD)
            return DEV_STATE_CD_ALWAYS;
        
        if(p->type == DEV_TYPE_HIDJ)
            return DEV_STATE_HID_ALWAYS;
        
        return DEV_STATE_ASSIGNED;
    }

    /* Device is assigned to a VM */
    assigned_uuid = VMid2uuid(p->VM);
    assert(assigned_uuid != NULL);
    
    if(VMuuid2id(vm_uuid) != p->VM)
    {
        /* Device is in use by another running VM */
        setBuffer(vm_assigned, assigned_uuid);
        return DEV_STATE_IN_USE;
    }

    setBuffer(vm_assigned, vm_uuid);

    /* Device is in use by specified VM */
    if(always_uuid != NULL)
        return DEV_STATE_THIS_ALWAYS;

    return DEV_STATE_THIS;
}


/* Query which devices the running VMs are using
 */
void queryExistingDevices(void)
{
    static IBuffer VMs = { NULL, 0, 0 };
    static IBuffer devs = { NULL, 0, 0 };
    int i, j;
    DevInfo *p;

    getVMs(&VMs);

    for(i = 0; i < VMs.len; ++i)
    {
        if(VMs.data[i] != 0 && VMs.data[i] != getUIVMid())
        {
            if(remote_query_devices(VMs.data[i], &devs) == 0)
            {
                LogInfo("Could not query devices for VM %d", VMs.data[i]);
            }
            else
            {
                LogInfo("VM %d reported using %d device%s", VMs.data[i], devs.len,
                        (devs.len == 1) ? "" : "s");
                
                /* Mark all devices in list as belonging to this VM */
                for(j = 0; j < devs.len; ++j)
                    claimDevice(devs.data[j], VMs.data[i]);
            }
        }
    }
}



/**** Functions to process high-level events (new devices, new VMs and UI commands) ****/

/* Process the given new device for assignment to a VM
 * param    dev_id               id of device to assign
 */
void assignNewDevice(int dev_id)
{
    DevInfo *device;   /* device to assign */
    int focus;         /* domid of VM in focus */
    const char *uuid;  /* uuid of target VM */

    device = findDevice(dev_id);

    if(device == NULL)
    {
        LogInfo("Cannot handle device %d, not found", dev_id);
        return;
    }

    if(device->type == DEV_TYPE_HIDU)
    {
        /* Device is a HiD, wait for interfaces to determine what variety */
        bindToDom0(device->sysName);
        return;
    }

    /* Send device to relevant VM */
    routeDevice(device, DEV_VM_NONE);
}


/* Process our devices for assignment to a new VM
 * param    dom_id               id of new VM
 */
void assignToNewVM(int dom_id)
{

    const char *uuid;  /* UUID of new VM */
    DevInfo *p;        /* Each device in system */

    assert(dom_id > 0);

    LogInfo("Adding new VM %d", dom_id);

    /* Convert domid to uuid */
    uuid = VMid2uuid(dom_id);

    if(uuid == NULL)
    {
        /* Could not find uuid for specified VM */
        LogInfo("VM %d claims to have started, but isn't running", dom_id);
        return;
    }

    int devgrab = has_device_grab(dom_id);

    for (p = devices; p != NULL; p = p->next) {
        if (alwaysCompare(p->Vid, p->Pid, p->serial, uuid) != 0) {
            moveDeviceToVM(p, uuid);
        }
        /* also assign if device grabbing vm & device is not assigned */
        else if (devgrab && (p->VM == DEV_VM_DOM0 || p->VM == DEV_VM_NONE)) {
            static Buffer nameBuf = { NULL, 0 };
            static Buffer fullNameBuf = { NULL, 0 };
            static Buffer assignedBuf = { NULL, 0 };
            int state;
            clearBuffer(&nameBuf);
            clearBuffer(&fullNameBuf);
            clearBuffer(&assignedBuf);
            state = getDeviceInfo(p->id, uuid, &nameBuf, &fullNameBuf, &assignedBuf);
            if (state != DEV_STATE_PLATFORM && state != DEV_STATE_HID_DOM0 && state != DEV_STATE_CD_DOM0 &&
                p->type != DEV_TYPE_HUB) {
                LogInfo("automatic assignment to %s: %s", uuid, fullNameBuf.data);
                moveDeviceToVM(p, uuid);
            } else {
                LogInfo("SKIP automatic assignment to %s: %s", uuid, fullNameBuf.data);
            }
        }
    }
}


/* Assign the given device to the specified existing VM
 * param    dev_id               id of device to assign
 * param    uuid                 UUID of VM to assign device to
 */
void assignToExistingVM(int dev_id, const char *uuid)
{
    DevInfo *device;   /* device to assign */

    device = findDevice(dev_id);

    if(device == NULL)
    {
        LogInfo("Cannot assign device %d, not found", dev_id);
        return;
    }

    alwaysClearOther(device->Vid, device->Pid, device->serial, uuid);
    moveDeviceToVM(device, uuid);
}


/* Unassign the given device from its VM and clear its always specification
 * param    dev_id               id of device to unassign
 */
void unassignFromVM(int dev_id)
{
    DevInfo *device;   /* device to unassign */

    device = findDevice(dev_id);

    if(device == NULL)
    {
        LogInfo("Cannot unassign device %d, not found", dev_id);
        return;
    }

    alwaysClear(device->Vid, device->Pid, device->serial);
    moveDeviceToVM(device, DOM0_UUID); 
}


/* Set (or clear) the sticky flag for the specified device
 * param    dev_id               id of device to assign
 * param    sticky               set (non-0) or clear (0) flag
 */
void setSticky(int dev_id, int sticky)
{
    const char *vm_uuid;  /* UUID of VM device is assigned to */
    DevInfo *p;  /* device */

    p = findDevice(dev_id);

    if(p == NULL)
    {
        /* No such device */
        LogInfo("Could not set sticky flag for device %d, not found", dev_id);
        return;
    }

    if(p->type == DEV_TYPE_HIDK || p->type == DEV_TYPE_HUB)
    {
        LogInfo("Cannot set sticky flag for device \"%s\", platform only device", p->name);
        return;
    }

    if(sticky == 0)
    {
        LogInfo("Clearing sticky flag for device \"%s\"", p->name);
        alwaysClear(p->Vid, p->Pid, p->serial);
        remote_report_dev_change(dev_id);
        return;
    }

    if(p->VM == DEV_VM_NONE || p->VM == DEV_VM_DOM0)
    {
        LogInfo("Cannot set sticky flag for device \"%s\", not assigned", p->name);
        return;
    }

    vm_uuid = VMid2uuid(p->VM);

    if(vm_uuid == NULL)
    {
        /* Cannot find VM assigned to */
        LogInfo("Cannot find VM %d that device \"%s\" is assigned to", p->VM, p->name);
        p->VM = DEV_VM_DOM0;
        moveDeviceToVM(p, DOM0_UUID);
        return;
    }

    LogInfo("Always assigning device \"%s\" to VM %d", p->name, p->VM);
    alwaysSet(p->Vid, p->Pid, p->serial, vm_uuid);
    remote_report_dev_change(dev_id);
}


/* Assign the given name to the specified device
 * param    dev_id               id of device to assign name to
 * param    name                 name to assign to device
 */
void setDeviceName(int dev_id, const char *name)
{
    DevInfo *p;  /* device */

    assert(name != NULL);

    p = findDevice(dev_id);

    if(p == NULL)
    {
        /* No such device */
        LogInfo("Could not give device %d name \"%s\", not found", dev_id, name);
        return;
    }

    LogInfo("Renaming device %d to \"%s\"", dev_id, name);
    setFakeName(p->Vid, p->Pid, p->serial, name);
    assert(p->name != NULL);
    free(p->name);
    p->name = strcopy(name);
    remote_report_dev_change(dev_id);
}


#if 0
/* Unbind any devices that have been assigned to dom0, at platform boot.
 * This is necessary because at start of day all devices get bound to dom0 before this daemon
 * is run and we disable auto binding.
 */
void purgeAccidentalBinds(void)
{
    DevInfo *p;   /* Each device in system */
    FILE *first;  /* Handle to first run flag file */

    first = fopen(FIRST_RUN_FILE, "r");

    if(first != NULL)
    {
        /* This isn't the first time we've run, don't purge */
        LogInfo("We've run before, not purging dom0 devices");
        fclose(first);
        return;
    }

    first = fopen(FIRST_RUN_FILE, "w");
    
    if(first == NULL)
    {
        LogError("Could not create %s", FIRST_RUN_FILE);
    }
    else
    {
        fclose(first);
    }

    LogInfo("Purging any accidental dom0 binds");

    for(p = devices; p != NULL; p = p->next)
    {
        if(p->VM == 0)
        {
            unbindFromDom0(p->sysName);
            p->VM = DEV_VM_NONE;
        }
    }
}
#endif
