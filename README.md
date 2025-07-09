# **ThinkFan C - Dynamic Fan Control for ThinkPads**  
*A lightweight, configurable fan control daemon for ThinkPad/Lenovo laptops on Linux.*  

---

## **📝 Features**  
✅ **Dynamic Fan Curve** - Adjust fan speeds based on CPU temperature  
✅ **Live Config Reload** - Modify `/etc/thinkfan_c/config.conf` without restarting  
✅ **Simple Syntax** - Define fan speeds for temperature thresholds  
✅ **Systemd Integration** - Runs as a background service  
✅ **Logging** - Logs to `journalctl` and `dmesg` for debugging  

---

## **⚙️ Installation**  

### **1. Install Dependencies (Arch Linux)**  
```bash
sudo pacman -S base-devel  # GCC, make, etc.
```

### 2. Clone repo
```bash
git clone git@github.com:Jirubizu/thinkfan_c.git
```

### **3. Compile & Install**  
```bash
sudo gcc -o /usr/local/bin/thinkfan_c main.c
sudo chmod +x /usr/local/bin/thinkfan_c
sudo mkdir -p /etc/thinkfan_c
```

### **4. Create Config File**  
```bash
sudo nano /etc/thinkfan_c/config.conf
```
Example config:  
```plaintext
(25, auto)    # Below 25°C → BIOS control
(40, 3)       # 40°C → Level 3
(50, 5)       # 50°C → Level 5
(60, 7)       # 60°C → Level 7 (max speed)
```

### **5. Set Up systemd Service**  
```bash
sudo nano /etc/systemd/system/thinkfan_c.service
```
Paste:  
```ini
[Unit]
Description=ThinkPad Fan Control
After=sysinit.target

[Service]
Type=simple
ExecStart=/usr/local/bin/thinkfan_c
Restart=always
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

### **6. Enable & Start**  
```bash
sudo systemctl daemon-reload
sudo systemctl enable --now thinkfan_c
```

---

## **🔧 Configuration**  

### **Config File Syntax**  
- **Format**: `(TEMPERATURE, SPEED)`  
- **Valid Speeds**:  
  - `auto` (BIOS control)  
  - `0`-`7` (manual speed levels)  

Example:  
```plaintext
(30, auto)
(45, 4)
(60, 7)
```

### **Reloading Config**  
- **Automatically**: The daemon checks for changes every 5 seconds.  
- **Manually**:  
  ```bash
  sudo touch /etc/thinkfan_c/config.conf  # Force reload
  ```

---

## **📊 Monitoring & Debugging**  

### **Check Logs**  
```bash
journalctl -u thinkfan_c -f  # Follow logs
dmesg | grep thinkfan_c  # Kernel logs
```

### **Verify Fan Control**  
```bash
cat /proc/acpi/ibm/fan  # Current fan status
```

### **Test Manually**  
```bash
sudo /usr/local/bin/thinkfan_c  # Run in foreground
```

---

## **🚨 Troubleshooting**  

### **Fan Control Not Working?**  
1. **Check kernel module**:  
   ```bash
   lsmod | grep thinkpad_acpi
   ```
2. **Verify fan permissions**:  
   ```bash
   ls -l /proc/acpi/ibm/fan
   ```
3. **Enable fan control**:  
   ```bash
   echo "options thinkpad_acpi fan_control=1" | sudo tee /etc/modprobe.d/thinkfan.conf
   sudo modprobe -r thinkpad_acpi
   sudo modprobe thinkpad_acpi
   ```

### **Config Not Reloading?**  
- Ensure the file has correct syntax.  
- Check logs for parsing errors.  

---

## **📜 License**  
MIT License - Free to use, modify, and distribute.  

---

## **💡 Credits**  
- Inspired by [thinkfan](https://github.com/vmatare/thinkfan)
- Designed for ThinkPad laptops running Linux  
