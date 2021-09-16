Aslan beacon测试站共有2块PCBA需要控制，1块为Pyramid Board，1块为ble_rssi_test_board(串口)。
具体控制逻辑和指令如下：有疑问或变更随时提出，谢谢！

测试项：
 
测试流程伪代码：

----enable beacon  & check FW version & get RSSI------------

1．	控制Pyramid Board-OUTPUT1 为高并等待一定时间t1
2．	控制Pyramid Board-I2C 依次发送5句cmd（第一句为检查DUT FW版本其中最后一句为enable cmd），并且获取I2C反馈，
反馈正常则继续，否则需重试r1次，超次数判fail；其中检查DUT FW版本部分，版本号会存在变更
3．	控制Pyramid Board-OUTPUT2 为高并等待一定时间t2（for dut reset）
4．	控制Pyramid Board-OUTPUT2 为低并等待一定时间t3（for dut reset）
5．	控制Pyramid Board-OUTPUT1 为低并等待一定时间t4
6．	控制ble_rssi_test_board，发送“scan_beacon\r\n”指令，等待0.1s，如果DUT beacon已使能，则反馈“scanned\r\n”，否则无反馈
【此处蓝牙处于scan状态，无法反馈状态】；发送“get_rssi\r\n”指令后等待0.1s，可获取“rssi 67\r\n”,其中67为RSSI的负值
（-67dBm）。注：此过程建议增加超时t5，和重试r2次两种机制，get_rssi存在记忆，此处判定规格待定：-25dBm  to -17dBm，
此部分作为limit判定

----disable beacon & check rssi-----------------------------------------

7．	重复步骤1.
8．	控制Pyramid Board-I2C 发送disable cmd，并且获取I2C反馈，反馈正常则继续，否则需重试r1次，超次数判fail
9．	控制Pyramid Board-OUTPUT1 为低并等待一定时间t6
10．	控制ble_rssi_test_board，发送“scan_beacon\r\n”指令，等待0.1s，如果DUT beacon已失能，则无反馈；反之判fail。
注：此过程建议增加超时t5，和重试r2次两种机制

其中：
Pyramid Board-OUTPUT1为控制DUT的thermal pin 高或低
Pyramid Board-OUTPUT2为控制DUT的scl pin 浮空或拉低
时间及次数参考值：t1=6s, r1=5, t2=1s, t3=6s, t4=6s, t5=1s, r2=10, t6=8s

步骤2中5句发给DUT的cmd（DUT器件地址0x42）：
# Tx rate set
0x0A, 0x04, 0x83, 0x00, 0x01, 0x65, 0x0D

# Tx power set
0x0A, 0x03, 0x82, 0x01, 0x4B, 0x0D

# Beacon data
0x0A, 0x1F, 0x86, 0x02, 0x1B, 0xFF, 0x01, 0xF1, 0xBE,
0xAC, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
0x1A, 0xEB, 0xEB, 0xEC, 0xDD, 0xC3, 0x00, 0x1A, 0x0D

# beacon enable
0x0A, 0x03, 0x84, 0x01, 0x4B, 0x0D

# read version
0x0A, 0x02, 0x01, 0x7d, 0x0d
此命令为读，反馈的数据版本格式解析等内容还未知

步骤8中发给DUT的disable cmd
# beacon disable
0x0A, 0x03, 0x84, 0x00, 0x02, 0x0D

