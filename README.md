# FundamentalUpdater Rev12 — Supertrend Pre-Buy Scanner

โปรแกรม Windows/C++ สำหรับคัดกรองหุ้นจากข้อมูล OHLC ใน AmiBroker เพื่อหารายชื่อหุ้นที่ยังไม่เกิด Supertrend Buy แต่ราคาอยู่ใกล้เส้น trigger ตามเกณฑ์ในโค้ด

**Rev12 ไม่ใช้ fundamental_v4.csv หรือไฟล์จาก FundamentalUpdater Rev5 ทุกกรณี** โปรแกรมดึงข้อมูลราคา OHLC จากฐานข้อมูล AmiBroker โดยตรง และไม่แก้ไขหรือพึ่งพา Rev5

## วิธีทำงาน

1. เชื่อมต่อ AmiBroker ผ่าน COM (Broker.Application)
2. ใช้ tools\export_amibroker_ohlc.js ส่งออกข้อมูล OHLC ล่าสุดไม่เกิน 120 แท่งต่อสัญลักษณ์จาก AmiBroker ไปที่ Data\ohlc_history.csv
3. คัดหุ้นที่ Supertrend (ATR 10, multiplier 3.0, source HL2) ยังเป็นขาลงและราคาอยู่ห่าง Buy trigger ไม่เกิน 1.0 ATR
4. กรองมูลค่าซื้อขายเฉลี่ย 20 แท่งก่อนหน้าให้มากกว่า 10 ล้านบาท และคะแนนเงื่อนไขฟื้นตัวอย่างน้อย 80
5. เขียนผลเรียงคะแนนจากมากไปน้อยที่ Data\near_buy_results.csv

**ข้อจำกัด:** “Pre-Buy” เป็นเพียงการคัดกรองหุ้นที่ราคาใกล้ trigger ไม่ใช่คำทำนายหรือการรับประกันว่าจะเกิดสัญญาณซื้อ ต้องตรวจกราฟและยืนยันด้วยข้อมูลราคาปิดด้วยตนเอง

## ความต้องการ

- Windows
- AmiBroker ติดตั้งและเปิดฐานข้อมูลที่มีข้อมูล OHLC
- C:\Windows\SysWOW64\CScript.exe สำหรับเรียก JScript ผ่าน Windows Script Host แบบ 32-bit
- ไม่จำเป็นต้องติดตั้ง FundamentalUpdater Rev5 และไม่ต้องมี fundamental_v4.csv

## วิธีใช้งาน (ไฟล์ EXE ที่ให้มา)

1. แตกแพ็กเกจทั้งโฟลเดอร์ โดยคงโครงสร้าง bin, tools และ source ไว้
2. เปิด AmiBroker และตรวจว่าฐานข้อมูลพร้อมใช้งาน
3. รัน bin\FundamentalUpdater_rev12.exe
4. โปรแกรมจะสร้างโฟลเดอร์ Data ให้อัตโนมัติ และบันทึกผลที่ Data\near_buy_results.csv

## Build จาก Source (Windows)

ต้องมี CMake 3.20+ และ compiler C++17 ที่รองรับ Windows เช่น MinGW-w64 หรือ MSVC

~~~powershell
cmake -S . -B build
cmake --build build --config Release
~~~

CMake จะวาง executable ไว้ใน bin\FundamentalUpdater_rev12.exe

## โครงสร้างโฟลเดอร์

~~~text
FundamentalUpdater_rev12/
├── CMakeLists.txt
├── README.md
├── bin/
│   └── FundamentalUpdater_rev12.exe
├── tools/
│   └── export_amibroker_ohlc.js
└── source/
    ├── main.cpp
    ├── SwingSignal.cpp
    ├── SwingSignal.h
    └── Config.h
~~~

เมื่อรันโปรแกรม โฟลเดอร์ Data จะถูกสร้างขึ้นเอง พร้อมไฟล์ OHLC และผลสแกน
