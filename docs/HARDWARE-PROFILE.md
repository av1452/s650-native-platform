# DSC-S650 consolidated hardware profile (round 9, 2026-11-03)

Built only from existing evidence, public documents and software analysis. **The camera was not opened, probed or contacted.** Every row carries its evidence class, kept strictly apart:

| Class | Meaning |
|---|---|
| **V** | Verified fact about *this* camera model or unit by a document specific to the S650 (manual, service manual) or by host-side USB observation of the user's unit |
| **T** | Teardown/photograph evidence from **another S650 unit** (iFixit). Does *not* prove the user's unit is identical |
| **F** | Family-level specification (COACH 8/9/10 briefs): what the *family* offers, not what the S650 uses |
| **O** | Evidence from other camera models/chips (a lead only) |
| **H** | Engineering hypothesis |
| **U** | Unknown |

## 1. Processor
| Item | Finding | Class | Source |
|---|---|---|---|
| Part | `ZR36473` = Zoran **COACH 9S**, shipped Jan 2007 | V-doc (about the part number, not the unit) | Zoran FY2007 Form 10-K (SEC) |
| Marking on a photographed S650 | `ZR36473BGCF`; the brief lists `ZR36473BGCG`; the meaning of F vs G is **unexplained**. A parts aggregator lists both (different stock levels, package metadata inconsistent) - not used. | T | iFixit; brief; aggregator listing |
| Is the user's unit's CPU confirmed? | **No.** USB enumeration and the manual's block structure agree with COACH 9 family features but do not identify the chip. | U for the unit | - |
| CPU | "32-bit MIPS CPU provides ease of development and porting of SW applications" (COACH 9 brief text; same wording in COACH 8 brief) | F | COACH 9 brief (saved locally: `zr36473_jiepei.html`) |
| Core | Not stated. Zoran licensed the **MIPS32 4KE family** (4KEc/4KEm/4KEp unspecified) in Aug 2002 for COACH and DTV processors, "transition away from Lexra" cores | O (2002 licence, not tied to 9S) | Design & Reuse; EE Times |
| ISA revision, endianness, FPU, MMU, caches, clock | Unknown | U | - |
| Dual CPU | Only the later COACH 10P brief says "Dual CPU architecture"; not stated for COACH 9/9S | O | COACH 10P brief |
| Software model | HCE (embedded host control) + "backward compatibility with previous COACH software API set" + "open firmware architecture for easy customer modification"; COACH-V adds "programmable DSP engine ... through downloadable firmware" (older generation) | F / O | COACH 9 and COACH-V briefs |
| Operating system | Not stated for any COACH 9 document | U | - |

## 2. RAM (new this round: datasheet read)
**Component seen in teardown photographs of another unit:** Qimonda `HYB25DC256160CE-5` (laser/ink marking transcribed in earlier rounds from iFixit images). **Confirmed for the user's unit: no.**
Datasheet used: Infineon/Qimonda "HYB25D256[40/80/16]0CE(L) / C[T/C/F]" **Rev. 1.2, Apr 2004** (85 pages; stored as `docs/sources/Infineon-Qimonda_HYB25D256x0C_DDR-SDRAM_datasheet_rev1.2_2004-04.pdf`; fetched via the aggregator's own redirect from datasheets.com, SHA-256 prefix `77b7852deaf9b94c`). A later revision (2.3, 2007-03) exists but was not retrievable as text.

| Parameter | Value | Page / section |
|---|---|---|
| Capacity | 256 Mbit = 268,435,456 bits | Overview 1.2 |
| Organisation (x16) | 4 banks x 8192 rows x 512 columns x 16 bits (= 16 Mbit x 16) | Block diagram (16 Mbit x 16) |
| Interface | DDR SDRAM, SSTL_2, differential clock CK/CK#, bidirectional DQS, DLL, data mask | Features |
| Speed grade `-5` | DDR400B (PC3200-3033): 200 MHz at CL3; 166 MHz at CL2.5; 133 MHz at CL2 | Table 1 / Table 2 |
| CAS-tRCD-tRP | 3-3-3 at 200 MHz | Table 2 |
| Supply | VDD = VDDQ = **2.6 V +/-0.1 V** for DDR400 (2.5 V +/-0.2 V for slower grades); VREF = VDDQ/2 | Features |
| Burst length / CAS latency | BL 2, 4, 8; CL 1.5 (DDR200 only), 2, 2.5, 3 | Features |
| Refresh | 7.8 us maximum average periodic refresh interval; auto and self refresh | Features |
| Package | P-TSOPII-66 or P-TFBGA-60 (12 x 8 mm) depending on suffix; the exact package of the photographed part is not established | Features / Table 2 |
| Initialisation | After stable power and clock: 200 us wait -> NOP, CKE high -> Precharge All -> EMRS (enable DLL) -> MRS (reset DLL + parameters) -> **200 clock cycles** -> Precharge All -> two Auto Refresh -> MRS (no DLL reset) | Section 2.1 |
| Row/column/bank address pins | A0-A12 (row), A0-A8 (column) + A10 auto precharge, BA0-BA1 | Pin description |

**Implications for the processor (class H unless stated):**
- The 16-bit data bus matches the family brief's "16-bits data DDR-SDRAM interface" (F) and the manual's `RAMD[0..15]` data lines (V, service manual block labels). That is **consistent**, not proof of part identity.
- The 14-bit address set `RAMA[0..14]` in the manual labels includes A0-A12 plus BA0/BA1 for a device of this organisation (V labels; mapping H).
- 32 MB total RAM (256 Mbit) is the only RAM size the photographs suggest; it is shared with image processing (H). Free RAM for our code is unknown (U).
- The memory controller initialisation above must be performed by the boot code before any DDR use; whoever runs first (boot ROM or first-stage loader) does it. **We must never assume RAM is initialised unless a loader is known to have done so** (this is why `hello_s650` uses no RAM-dependent runtime).

## 3. NAND flash
**Component seen in teardown photographs of another unit:** Hynix `HY27US08561A` (256 Mbit, x8, 3.3 V class, TSOP-48 per earlier research). **Confirmed for the user's unit: no.** The exact `HY27US08561A` datasheet remains unread (inaccessible aggregator pages recorded in earlier rounds). Geometry below is **inferred from sibling Hynix datasheets** (`HY27US08561M`, `HY27US08121A/B`, copies saved locally in an earlier round) and is labelled H.

| Parameter | Value | Class |
|---|---|---|
| Density / width | 256 Mbit, 32M x 8 | confirmed for the part number (earlier aggregator listing), H for the unit |
| Page | 528 bytes = 512 main + 16 spare | H (sibling) |
| Block | 32 pages = 16 KiB main (+ 512 B spare) | H |
| Device | 2048 blocks | H |
| Capacity incl. spare | 33,554,432 B main + 1,048,576 B spare | H |
| Factory bad blocks | marked in the spare area of the first page of the block (sibling datasheets) | H |
| ECC | Required by vendors (1-bit/512 B class for this generation of SLC); the **camera's** ECC algorithm and spare-area layout are unknown | U |
| Device ID | manufacturer `ADh`; device code `75h` expected for this density (pattern from siblings) - **unconfirmed** | H |
| Interface | 8 data lines `MD[0..7]`, `CLE`, `ALE`, `CE0`, `RE`, `WE`, `WP`, `R/B` (service-manual block labels) | V (labels) |
| Firmware-storage implication | Raw capacity 33,554,432 B vs the 25,263,616 B internal volume seen over USB leaves about 8.3 MB for everything else (firmware, loader, settings, FAT overhead, bad-block reserve). Consistent with sharing the NAND; **not proof**, and **the volume size is not evidence of where firmware lives** | V (volume size) / H |

## 4. Display and other subsystems (service manual labels, V)
| Subsystem | Labels / facts | Notes |
|---|---|---|
| LCD | Panel documented as 2.0-inch, "115,200 dots (480x240)" (manual). Lines `LCD[0..7]` plus sync/clock labelled; connects via the long bottom connector (silkscreen J3/40) | 8 data lines; format unknown |
| CCD front end | `CCD[0..11]` (12-bit), `TG_EN`, `CDS_EN`, `AFERST` | timing generator and AFE controls exist as labels |
| Lens | `LENS_*` plus zoom/focus/iris/shutter motor lines; 18-contact lens ribbon J1 | driven by the main board; code unknown |
| Flash | `STROBE_*`, `FLASH_VP`; separate STROBE board | **hazardous circuit; no handling instructions given** |
| Keys | `SW_KEY_ADC`, `MODE_R0..5` -> keys read through an ADC and a mode-dial resistor ladder (H from names) | mechanism for software access unknown |
| Storage | `MS_*` Memory Stick signals | - |
| USB | `D+`, `D-`; host observation: full speed, `054c:0295` (Mass Storage, INQUIRY vendor "SONY", FAT12 volume 25,263,616 B), `054c:0296` (PictBridge/PTP), **no vendor-specific interface**, PTP request timed out | V (observed on the user's unit by host enumeration) |
| Power rails named in the diagram | `VDDCORE`, `VDDDDR`, `VDDUSB`, `VDDUSBPLL`, `VDDDAC`, `VDDADC`, `VDDAUO`, `VDDSPKR`; part `AO8803` (Q17). Rail voltages are not given in the extracted text. | V (labels); DDR rail = 2.6 V implied by the RAM datasheet if the -5 grade is used (H) |
| Fuses | F1 1.6 A, F2 800 mA under "MCU BOARD" | V |
| Debug interfaces | **Absent from the extracted text:** `ICE_*`, JTAG, UART, BOOT, DBG, `TP##` test points, `CN##` connectors. A MIPS core usually contains EJTAG logic; whether it is bonded out is unknown. | V (absence in text) / U |
| Servicing | Replacement-only: the manual says to contact Sony for lens/main-board failures and to erase internal memory before replacement; revision history is mechanical only | V |
| New clue worth recording | The block diagram groups `MEMORY` (NAND, 8-bit) separately from `DDR` (16-bit), consistent with the family's "direct interface to resident flash (NAND or NOR)" and no separate boot-ROM part named in the diagram. The manual does not show a NOR part. | V (labels) / H |

## 5. Other photographs and teardown material
Searches this round (exact marking `ZR36473BGCF/BGCG` with teardown/PCB terms; S650 teardown/firmware terms) returned **no additional board photographs or teardowns**; the only sources remain the iFixit S650 pages (device page and lens-replacement guide 9995) already analysed in round 3, which are of **another unit**. Distributor listings for the part number exist but carry no teardown data.

## 6. Summary of what is established versus not
| Established (V) | Family-level (F) | Hypotheses (H) | Unknown (U) |
|---|---|---|---|
| 480x240-dot panel; Mass Storage/PictBridge/PTP, full-speed USB, no vendor interface; ~24 MB internal volume; Memory Stick Duo; 8-bit NAND bus, 16-bit DDR bus labels; no documented debug connection; replacement-only servicing | 32-bit MIPS CPU; DDR/NAND/NOR; LCD controller; OSD with transparency; HW picture-in-picture; HCE; JPEG/MJPEG codec | Hynix NAND and Qimonda DDR in the user's unit; 4KE-class core; sharing of NAND by firmware; non-square 480x240 pixels | core, ISA, endianness, boot path, memory map, firmware location/format, display programming, button access, all camera-function APIs, any loader, any recovery method |
