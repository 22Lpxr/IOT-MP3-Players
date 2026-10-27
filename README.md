Physical Computing Project 2026 - IT KMITL

# IOT-MP3-Players

## Member
- วีรชัย หล่อปา 68070175
- เมธาวินทร์ โชติช่วง 68070151
- พชรพล เจริญ 68070113

## Project Overview & Objectives
โปรเจกต์ **IOT-MP3-Players** เป็นเครื่องเล่นเพลง MP3 อัจฉริยะที่เชื่อมต่อกับอินเทอร์เน็ตผ่านระบบ IoT (Internet of Things) โดยใช้โปรโตคอล MQTT ในการรับส่งคำสั่งและสถานะการทำงานแบบ Real-time ร่วมกับไมโครคอนโทรลเลอร์, โมดูลเล่นเพลง DFPlayer Mini, หน้าจอ LCD I2C และฮาร์ดแวร์ควบคุมต่างๆ

### จุดประสงค์ของโปรเจกต์ (Objectives)
1. ประยุกต์ใช้ความรู้ด้าน Physical Computing และ Internet of Things (IoT) ในการสร้างอุปกรณ์ฮาร์ดแวร์ที่สามารถควบคุมผ่านเครือข่ายได้
2. พัฒนาระบบเครื่องเล่นเพลงที่สามารถควบคุมได้ทั้งจากปุ่มกดทางกายภาพ (Physical Buttons), โพเทนชิอมิเตอร์สำหรับปรับระดับเสียง และสั่งการผ่านระบบ Web/MQTT Remote Control
3. แสดงผลสถานะการทำงาน ชื่อเพลง และระดับเสียงปัจจุบันผ่านหน้าจอ LCD และไฟสถานะ LED

---

## Detailed Features & System Description

### 1. Hardware Components
- **Microcontroller & Wi-Fi:** เชื่อมต่ออินเทอร์เน็ตผ่าน Wi-Fi เพื่อสื่อสารกับ MQTT Broker (`broker.hivemq.com`)
- **DFPlayer Mini:** โมดูลสำหรับเล่นไฟล์เสียง MP3 จาก SD Card รองรับการเปลี่ยนเพลงและการปรับ Equalizer (EQ)
- **LCD 16x2 (I2C):** แสดงชื่อเพลงปัจจุบัน สถานะ (Playing/Paused) และระดับเสียง (Volume)
- **Status LEDs:** หลอดไฟ LED 3 สี แสดงสถานะของระบบ
  - **สีแดง:** ยังไม่ได้เชื่อมต่อ Wi-Fi / MQTT หรือเกิดข้อผิดพลาด
  - **สีน้ำเงิน:** กำลังเล่นเพลง (Playing)
  - **สีเหลือง:** พักการเล่นเพลง (Paused)
- **Inputs:**
  - ปุ่มกด (Buttons) 3 ปุ่ม สำหรับควบคุม Previous, Play/Pause และ Next
  - โพเทนชิอมิเตอร์ (Potentiometer) สำหรับปรับระดับเสียง (Volume 0-100)

### 2. MQTT Communication & Web Integration
ระบบใช้โปรโตคอล MQTT ผ่าน Topic หลัก `mini-mp3-7421` (สามารถปรับเปลี่ยนได้ตามการใช้งาน) ประกอบด้วย:
- **`mini-mp3-7421/cmd`:** รับคำสั่งควบคุมจากภายนอก เช่น `toggle`, `prev`, `next`, `play:n`, หรือ `eq:n`
- **`mini-mp3-7421/status`:** ส่งข้อมูลสถานะปัจจุบัน (เพลงที่เล่น, ชื่อเพลง, สถานะการเล่น, ระดับเสียง, โหมด EQ) แบบ JSON กลับไปยังระบบ
- **`mini-mp3-7421/playlist`:** ส่งรายชื่อเพลงทั้งหมดในระบบ
- **`mini-mp3-7421/online`:** แสดงสถานะการเชื่อมต่อของอุปกรณ์ (Last Will and Testament)

---

## Libraries Used
- **LiquidCrystal_I2C** (ควบคุมจอ LCD ผ่าน I2C)
- **DFRobotDFPlayerMini** (ควบคุมโมดูลเล่นเพลง MP3)
- **WiFi / WiFiS3** (เชื่อมต่อเครือข่ายอินเทอร์เน็ตไร้สาย)
- **PubSubClient** (จัดการรับส่งข้อความผ่าน MQTT)
- **Wire** (สื่อสาร I2C พื้นฐาน)

---

## Getting Started
1. ติดตั้งไลบรารีที่จำเป็นทั้งหมดใน Arduino IDE
2. ตั้งค่า `WIFI_SSID` และ `WIFI_PASS` ในโค้ดให้ตรงกับเครือข่ายของคุณ
3. อัปโหลดโค้ดลงบนบอร์ดไมโครคอนโทรลเลอร์
4. ตรวจสอบการเชื่อมต่อผ่าน Serial Monitor (ความเร็ว 115200) และหน้าจอ LCD