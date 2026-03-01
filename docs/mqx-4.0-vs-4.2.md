# MQX 4.0 vs 4.2 — Differences et migration firmware SC944D

Le firmware BP_MQX_ETH a ete developpe sur MQX 3.8/4.0 avec CodeWarrior.
Ce document recense les APIs utilisees, verifie leur presence dans MQX 4.2
(source GitHub), et liste les differences connues pour la migration GCC.

## 1. APIs critiques — verification dans MQX 4.2

### Kernel (mqx/mqx/source/)

| API | Header MQX 4.2 | Present | Notes |
|-----|----------------|---------|-------|
| `MQX_INITIALIZATION_STRUCT` | `include/mqx.h` | Oui | Structure identique, `MQX_VERSION = 420` |
| `_mqx_set_cpu_type()` | `include/mqx.h` | Oui | Inchange |
| `_task_create()` | `include/mqx.h` | Oui | Inchange |
| `_task_destroy()` | `include/mqx.h` | Oui | Inchange |
| `_task_get_id()` | `include/mqx.h` | Oui | Inchange |
| `TASK_TEMPLATE_STRUCT` | `include/mqx.h` | Oui | Inchange |
| `_time_delay()` | `include/mqx.h` | Oui | Inchange |
| `_time_get()` | `include/mqx.h` | Oui | Inchange |
| `_time_set()` | `include/mqx.h` | Oui | Inchange |
| `_time_notify_kernel()` | `include/mqx.h` | Oui | Inchange |
| `_mutex_init()` | `include/mutex.h` | Oui | Inchange |
| `_mutex_lock()` | `include/mutex.h` | Oui | Inchange |
| `_mutex_unlock()` | `include/mutex.h` | Oui | Inchange |
| `_mutex_destroy()` | `include/mutex.h` | Oui | Inchange |
| `_event_create()` | `include/event.h` | Oui | Inchange |
| `_event_open()` | `include/event.h` | Oui | Inchange |
| `_event_wait_all()` | `include/event.h` | Oui | Inchange |
| `_event_set()` | `include/event.h` | Oui | Inchange |
| `_lwevent_create()` | `include/lwevent.h` | Oui | Nouvelle alternative legere |
| `_lwsem_create()` | `include/lwsem.h` | Oui | Nouvelle alternative legere |
| `_int_install_isr()` | `include/mqx.h` | Oui | Inchange |
| `_psp_int_init()` | PSP ColdFire | Oui | Inchange |

### I/O Subsystem (mqx/mqx/source/io/)

| API | Module | Present | Notes |
|-----|--------|---------|-------|
| `_io_init()` | `io_init.c` | Oui | Inchange |
| `_io_fopen()` | `include/fio.h` | Oui | Inchange |
| `_io_fclose()` | `include/fio.h` | Oui | Inchange |
| `_io_read()` | `include/fio.h` | Oui | Inchange |
| `_io_write()` | `include/fio.h` | Oui | Inchange |
| `_io_ioctl()` | `include/fio.h` | Oui | Inchange |
| `_mcf52xx_uart_serial_int_install()` | `io/serial/` | Oui | Inchange |
| `_mcf52xx_uart_serial_polled_install()` | `io/serial/` | Oui | Inchange |
| `_mcf52xx_i2c_polled_install()` | `io/i2c/` | Oui | Inchange |
| `_mcf5xxx_qspi_polled_install()` | `io/spi_legacy/` | Oui | **Attention** : `spi_legacy/` |
| `_io_adc_install()` | `io/adc/` | Oui | Inchange |
| `_io_gpio_install()` | `io/gpio/` | Oui | Inchange |
| `_io_flashx_install()` | `io/flashx/` | Oui | Inchange |

### RTCS / TCP-IP (mqx/rtcs/source/)

| API | Header MQX 4.2 | Present | Notes |
|-----|----------------|---------|-------|
| `RTCS_create()` | `include/rtcs.h` | Oui | Signature identique : `uint32_t RTCS_create(void)` |
| `RTCS_socket()` | `include/rtcs_sock.h` | Oui | Macro: `#define socket RTCS_socket` |
| `bind()` | `include/rtcs_sock.h` | Oui | **Changement** : signature BSD-like `bind(sock, addr, addrlen)` |
| `listen()` | `include/rtcs_sock.h` | Oui | `uint32_t listen(uint32_t, int32_t)` |
| `accept()` | `include/rtcs_sock.h` | Oui | `uint32_t accept(uint32_t, sockaddr*, uint16_t*)` |
| `send()` | `include/rtcs_sock.h` | Oui | `int32_t send(uint32_t, void*, uint32_t, uint32_t)` |
| `recv()` | `include/rtcs_sock.h` | Oui | `int32_t recv(uint32_t, void*, uint32_t, uint32_t)` |
| `connect()` | `include/rtcs_sock.h` | Oui | `uint32_t connect(sock, addr, addrlen)` |
| `select()` | `include/rtcs_sock.h` | Oui | `int32_t select(nfds, readfds, writefds, exceptfds, ...)` |
| `sendto()` / `recvfrom()` | `include/rtcs_sock.h` | Oui | Signatures BSD-like |
| `closesocket()` | `include/rtcs_sock.h` | Oui | Inchange |
| `shutdownsocket()` | `include/rtcs_sock.h` | Oui | Inchange |
| `ipcfg_init_device()` | `include/ipcfg.h` | Oui | Inchange |
| `DHCP_lease_*()` | `include/dhcp.h` | Oui | Inchange |
| `DNS_*()` | `include/dnscln.h` | Oui | Inchange |
| `HTTPSRV_*()` | `include/httpsrv.h` | Oui | API refactoree en 4.2 |

### Ethernet / PHY

| API | Header MQX 4.2 | Present | Notes |
|-----|----------------|---------|-------|
| `ENET_initialize()` | `include/enet.h` | Oui | Inchange |
| `phy_dp83xxx_IF` | `io/enet/phy/phy_dp83xxx.h` | Oui | Pour DP83640TVV (SC944D) |
| `MCF5xxx_FEC_IF` | `io/enet/mcf5xxx_fec/` | Oui | Inchange |

## 2. Differences connues MQX 4.0 → 4.2

### 2.1 Types entiers (critique)

MQX 4.2 utilise les types C99 standard. Le firmware legacy utilise les anciens types :

| Ancien (MQX 3.x/4.0) | Nouveau (MQX 4.2) | Header |
|-----------------------|--------------------|--------|
| `uint_32` | `uint32_t` | `<stdint.h>` |
| `uint_16` | `uint16_t` | `<stdint.h>` |
| `uint_8` | `uint8_t` | `<stdint.h>` |
| `int_32` | `int32_t` | `<stdint.h>` |
| `boolean` | `bool` | `<stdbool.h>` |
| `pointer` | `void *` | natif |
| `_mqx_uint` | `_mqx_uint` | Inchange (typedef → `uint32_t`) |
| `char_ptr` | `char *` | natif |
| `uchar` | `unsigned char` | natif |

**Action** : sed/replace global dans le firmware ou ajouter un header de compatibilite.

### 2.2 Socket API

MQX 4.2 a aligne les sockets sur les signatures BSD :
- `bind(sock, sockaddr*, addrlen)` au lieu de `bind(sock, sockaddr*, sizeof)`
- `accept()` retourne un socket descriptor (comme BSD) au lieu d'un code d'erreur
- `select()` utilise `rtcs_fd_set` au lieu de `RTCS_selectset()`
- `closesocket()` et `shutdownsocket()` au lieu de `shutdown()`

**Action** : verifier chaque appel socket dans le firmware et adapter les signatures.

### 2.3 SPI Driver

MQX 4.2 a deux implémentations SPI :
- `io/spi/` — nouveau driver SPI generalise
- `io/spi_legacy/` — ancien driver (compatible MQX 4.0)

Le BSP m52259evb utilise `_mcf5xxx_qspi_polled_install()` qui est dans `spi_legacy/`.
Il faut compiler `spi_legacy/` et non `spi/` pour maintenir la compatibilite.

### 2.4 HTTP Server

MQX 4.2 remplace l'ancien HTTPD par `HTTPSRV` :
- Nouvelle API : `HTTPSRV_init()`, `HTTPSRV_release()`
- CGI callbacks : structure `HTTPSRV_CGI_REQ_STRUCT` differente
- SSI remplace par des variables dans les templates

**Action** : le handler HTTP du firmware devra etre adapte a la nouvelle API HTTPSRV.

### 2.5 Ethernet / ipcfg

MQX 4.2 ajoute le support IPv6 et reorganise `ipcfg` :
- `ipcfg_init_device()` inchange pour IPv4
- Nouvelles fonctions `ipcfg6_*` pour IPv6 (optionnel)
- `ENET_get_mac_address()` → inchange

### 2.6 Memory allocator

MQX 4.2 ajoute `TLSF` (Two-Level Segregated Fit) comme allocateur alternatif :
- `tlsf.h`, `tlsf_adaptation.h`
- L'allocateur par defaut reste le meme
- Utile si des problemes de fragmentation memoire apparaissent (64 KB SRAM)

### 2.7 PSP ColdFire

Le PSP ColdFire est stable entre 4.0 et 4.2 :
- `dispatch.S` : context switching, inchange pour MCF52259
- `psp_supp.c` : support CPU, inchange
- Nouveaux defines CPU dans `mqx_cpudef.h`

## 3. Include paths pour le Makefile GCC

```makefile
# MQX Kernel headers
MQX_INCLUDE = mqx/mqx/source/include

# PSP ColdFire (processor support)
PSP_INCLUDE = mqx/mqx/source/psp/coldfire

# BSP m52259evb (SC944D)
BSP_INCLUDE = bp/bsp/m52259evb

# I/O drivers
IO_INCLUDE_BASE = mqx/mqx/source/io
IO_SERIAL    = $(IO_INCLUDE_BASE)/serial
IO_I2C       = $(IO_INCLUDE_BASE)/i2c
IO_SPI       = $(IO_INCLUDE_BASE)/spi_legacy
IO_GPIO      = $(IO_INCLUDE_BASE)/gpio
IO_LWGPIO    = $(IO_INCLUDE_BASE)/lwgpio
IO_ADC       = $(IO_INCLUDE_BASE)/adc
IO_FLASHX    = $(IO_INCLUDE_BASE)/flashx
IO_ENET      = $(IO_INCLUDE_BASE)/enet
IO_ENET_PHY  = $(IO_INCLUDE_BASE)/enet/phy
IO_ENET_FEC  = $(IO_INCLUDE_BASE)/enet/mcf5xxx_fec
IO_RTC       = $(IO_INCLUDE_BASE)/rtc
IO_INT_CTRL  = $(IO_INCLUDE_BASE)/int_ctrl
IO_TIMER     = $(IO_INCLUDE_BASE)/timer
IO_PCB       = $(IO_INCLUDE_BASE)/pcb
IO_MEM       = $(IO_INCLUDE_BASE)/io_mem
IO_NULL      = $(IO_INCLUDE_BASE)/io_null

# RTCS (TCP/IP stack)
RTCS_INCLUDE = mqx/rtcs/source/include

# CFLAGS include paths
INCLUDES = \
  -I$(MQX_INCLUDE) \
  -I$(PSP_INCLUDE) \
  -I$(BSP_INCLUDE) \
  -I$(IO_SERIAL) \
  -I$(IO_I2C) \
  -I$(IO_SPI) \
  -I$(IO_GPIO) \
  -I$(IO_LWGPIO) \
  -I$(IO_ADC) \
  -I$(IO_FLASHX) \
  -I$(IO_ENET) \
  -I$(IO_ENET_PHY) \
  -I$(IO_ENET_FEC) \
  -I$(IO_RTC) \
  -I$(IO_INT_CTRL) \
  -I$(IO_TIMER) \
  -I$(IO_PCB) \
  -I$(IO_MEM) \
  -I$(IO_NULL) \
  -I$(RTCS_INCLUDE)
```

## 4. Composants MQX a compiler

| Composant | Chemin | Fichiers (approx) | Requis |
|-----------|--------|--------------------|--------|
| PSP ColdFire | `mqx/mqx/source/psp/coldfire/` | ~90 (.c + .S) | Oui |
| Kernel | `mqx/mqx/source/kernel/` | ~60 | Oui |
| BSP m52259evb | `bp/bsp/m52259evb/` | 17 | Oui |
| I/O serial | `mqx/mqx/source/io/serial/` | ~10 | Oui |
| I/O I2C | `mqx/mqx/source/io/i2c/` | ~6 | Oui |
| I/O SPI legacy | `mqx/mqx/source/io/spi_legacy/` | ~6 | Oui |
| I/O GPIO | `mqx/mqx/source/io/gpio/` | ~4 | Oui |
| I/O lwGPIO | `mqx/mqx/source/io/lwgpio/` | ~2 | Oui |
| I/O ADC | `mqx/mqx/source/io/adc/` | ~4 | Oui |
| I/O FlashX | `mqx/mqx/source/io/flashx/` | ~4 | Oui |
| I/O Ethernet | `mqx/mqx/source/io/enet/` | ~20 | Oui |
| I/O int_ctrl | `mqx/mqx/source/io/int_ctrl/` | ~2 | Oui |
| I/O RTC | `mqx/mqx/source/io/rtc/` | ~2 | Oui |
| I/O timer | `mqx/mqx/source/io/timer/` | ~2 | Oui |
| RTCS | `mqx/rtcs/source/` | ~120 | Oui |

## 5. Risques identifies

1. **Types entiers** : le firmware utilise `uint_32`, `boolean`, etc. — remplacement global requis
2. **Socket API** : signatures modifiees pour conformite BSD — verifier chaque appel
3. **HTTP server** : API HTTPSRV differente — rewrite des handlers CGI probable
4. **SPI** : utiliser `spi_legacy/` et non `spi/` pour compatibilite
5. **Linker script** : le script CW n'est pas compatible GCC — a creer (TASK-003)
6. **Assembleur** : `dispatch.S` et startup ColdFire utilisent la syntaxe CW — a adapter pour GAS

## References

- MQX 4.2 source : https://github.com/Mr-Shannon/Freescale_MQX_4_2
- AN4190 Porting MQX BSP : https://nxp.com/docs/en/application-note/AN4190.pdf
- MQX 4.2 Release Notes : `mqx/doc/`
- Hardware SC944D : `essensys-doc/archi/hardware-sc944d.md`
- GPIO mapping : `essensys-doc/archi/legacy-client-config.md`
