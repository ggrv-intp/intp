# IntP - Application Interference Measurement Tool

**SystemTap-based tool for measuring application interference in Linux systems**

[![Kernel](https://img.shields.io/badge/kernel-6.8.0--90-blue)](https://kernel.org/)
[![SystemTap](https://img.shields.io/badge/SystemTap-5.2-green)](https://sourceware.org/systemtap/)
[![Ubuntu](https://img.shields.io/badge/Ubuntu-24.04_LTS-orange)](https://ubuntu.com/)

---

## 🚀 Quick Start

### Consumer CPUs (Intel Core i5/i7/i9, AMD)

```bash
cd ~/Documents/intp

# Run IntP (6 out of 7 metrics functional)
sudo stap -g -B CONFIG_MODVERSIONS=y intp-6.8.stp firefox

# View metrics in another terminal
watch -n2 -d cat /proc/systemtap/stap_*/intestbench
```

### Intel Xeon CPUs (with RDT support)

```bash
# Start LLC monitoring helper
./intp-resctrl-helper.sh start

# Run IntP (all 7 metrics functional)
sudo stap -g -B CONFIG_MODVERSIONS=y intp-resctrl.stp firefox

# View metrics
watch -n2 -d cat /proc/systemtap/stap_*/intestbench

# Stop helper when done
./intp-resctrl-helper.sh stop
```

---

## 📊 Metrics Measured

| Metric | Acronym | Description | Consumer CPUs | Xeon with RDT |
|--------|---------|-------------|---------------|---------------|
| Network Physical | `netp` | Network layer utilization | ✅ | ✅ |
| Network Stack | `nets` | Network stack utilization | ✅ | ✅ |
| Block I/O | `blk` | Disk I/O utilization | ✅ | ✅ |
| Memory Bandwidth | `mbw` | Memory bandwidth usage | ✅ | ✅ |
| LLC Miss Ratio | `llcmr` | Cache miss percentage | ✅ | ✅ |
| **LLC Occupancy** | `llcocc` | Cache occupancy | ❌ (0) | ✅ |
| CPU Utilization | `cpu` | CPU usage percentage | ✅ | ✅ |

**Example Output:**
```
netp    nets    blk     mbw     llcmr   llcocc  cpu
02      01      05      12      03      00      45
```

---

## 🔄 What Changed for Kernel 6.8.0

| Component | Original (≤6.6) | Kernel 6.8.0+ | Reason |
|-----------|-----------------|---------------|---------|
| **MSR Definitions** | Defined in script | Removed | Now in kernel headers |
| **LLC Occupancy** | perf events (cqm_rmid) | resctrl filesystem | API removed from kernel |
| **Module Loading** | `stap -g` | `stap -g -B CONFIG_MODVERSIONS=y` | MODVERSIONS enforced |
| **SystemTap Version** | 4.9-5.0 (package) | 5.2+ (source) | Kernel 6.8 compatibility |

**New Files:**
- `intp-6.8.stp` - Patched for 6.8.0 (LLC disabled)  
- `intp-resctrl.stp` - Full version with resctrl  
- `intp-resctrl-helper.sh` - LLC monitoring daemon

---

## 📖 Documentation

| Document | Description |
|----------|-------------|
| **[Installation Guide](install/install_ubuntu24_desktop.md)** | Complete Ubuntu 24.04 setup |
| **[Quick Fix: Module Loading](docs/SYSTEMTAP-MODULE-ISSUE.md)** | Solve "Invalid module format" |
| **[Kernel Changes](docs/KERNEL-6.8-NOTES.md)** | API compatibility details |
| **[LLC Monitoring](docs/LLC-OCCUPANCY-RESCTRL.md)** | resctrl implementation |
| **[Complete Summary](docs/FINAL-SUMMARY.md)** | Full project overview |

---

## 🛠️ Installation (Quick Version)

### 1. Build SystemTap 5.2

```bash
sudo apt install -y build-essential git libdw-dev libelf-dev gettext
cd ~/Documents && git clone https://sourceware.org/git/systemtap.git
cd systemtap && git checkout release-5.2
./configure --prefix=/usr/local --disable-docs && make -j$(nproc)
sudo make install
```

### 2. Install Debug Symbols

```bash
sudo apt install -y ubuntu-dbgsym-keyring
echo "deb http://ddebs.ubuntu.com noble main restricted universe multiverse" | sudo tee /etc/apt/sources.list.d/ddebs.list
sudo apt update && sudo apt install -y linux-image-$(uname -r)-dbgsym
```

### 3. Test

```bash
sudo stap -g -B CONFIG_MODVERSIONS=y -e 'probe begin { printf("OK\n") exit() }'
```

**Full guide:** [install/install_ubuntu24_desktop.md](install/install_ubuntu24_desktop.md)

---

## 🐛 Common Issues

### "Invalid module format"
```bash
# Add -B CONFIG_MODVERSIONS=y
sudo stap -g -B CONFIG_MODVERSIONS=y intp-6.8.stp firefox
```

### "MSR redefined" or "cqm_rmid" errors
```bash
# Use patched version
sudo stap -g -B CONFIG_MODVERSIONS=y intp-6.8.stp firefox
```

### LLC Occupancy = 0
- **Consumer CPUs**: Expected (no RDT support)
- **Xeon CPUs**: Use `intp-resctrl.stp` + helper daemon

---

## 🤝 Credits

**Original IntP:** [@mclsylva](https://github.com/mclsylva), [@superflit](https://github.com/superflit)  
**Kernel 6.8.0 Adaptation:** Project contributors

---

*Kernel: 6.8.0-90 | SystemTap: 5.2 | Updated: Jan 2026*
