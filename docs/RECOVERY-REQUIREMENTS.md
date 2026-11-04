# Recovery requirements: conditions before any persistent modification (round 8)

**The camera is a spare development unit, but it is not disposable.** Nothing in this phase reads or writes camera storage, and this document does not authorise it. It states the conditions that must hold *before* any operation that could modify firmware or persistent memory, and what a recovery procedure must be shown to do.

A backup is not a recovery method. A raw NAND dump is not automatically a usable firmware image. A successful read is not proof that a restore works.

## What is known that affects recovery
| Topic | Known | Grade |
|---|---|---|
| Probable flash | A Hynix `HY27US08561A` 256 Mbit NAND (TSOP-48) on a *photographed other-unit* board; not confirmed on this unit. Geometry (528 B pages = 512 + 16 spare, 32 pages/block, 2048 blocks), factory bad blocks marked in spare bytes, ECC needed: inferred from **sibling** Hynix datasheets; the exact datasheet was not read. | P / I |
| Interface | 8-bit NAND with R/B and control lines (service manual signal names). | C (text) |
| Programmer support | A programmer vendor's listing shows support for a 63-ball BGA variant of the part family (not necessarily this package). | I |
| Camera's ECC / spare layout / bad-block handling | **Unknown** | U |
| Firmware location and size, boot layout, bootloader preservation | **Unknown** | U |
| Any recovery or update-recovery mode | **None documented**; none may be entered without approval | U |
| Number of units | One spare unit; a second unit would make rehearsal safer | - |

## Required prerequisites (all must be met, in this order)
1. **Evidence first.** Documents or an image analysis that explain what is stored where (`FIRMWARE-ANALYSIS.md`). Without it, a "backup" is just a blob.
2. **A read-only acquisition method, approved in writing by the user**, with a written procedure, the exact equipment, and a qualified person to execute it. No method is proposed here.
3. **Verification of the acquisition:** at least two independent full reads produce byte-identical results (including spare areas); SHA-256 recorded; bad-block table and ECC behaviour characterised.
4. **Understanding of the data:** spare-area layout, ECC algorithm, bad-block markers and the camera's remapping; bootloader region identified so it can be preserved.
5. **A restoration procedure written down and rehearsed**, with a defined success test (camera boots, shoots, plays back, USB works, settings intact, image hashes unchanged).
6. **Independent validation on a different unit:** a restore must be demonstrated on a donor *other than* the unit under experiment, or - if only one unit exists - the plan must state clearly that a failed first restore ends the project for that unit.
7. **Recovery equipment on hand** before the first write: programmer, adapters, known-good chips if relevant.
8. **A stop rule:** any unexplained difference between a read-back and the original halts all further work.
9. **Explicit approval** for each physical step, after the technical purpose, risks and safer alternatives have been stated.

## Who must do the physical work
A qualified electronics technician with experience in NAND/TSOP/BGA work, using appropriate programming equipment. **Opening the camera is hazardous because it contains a high-voltage flash circuit; this project gives no instructions for opening it or for handling or discharging that circuitry.** The user should not attempt hands-on steps on the basis of this repository.

## What must NOT be done (this phase and until all prerequisites are met)
Write, erase or program NAND, NOR, EEPROM, or any persistent memory; send undocumented USB commands; enter undocumented service modes; probe unidentified pads; run firmware update utilities; bypass access controls; modify the original firmware.

## Safer alternatives that give evidence without modification
Written enquiries to Sony and to the successor of Zoran; analysis of any lawfully obtained image; visual identification of chip markings by a qualified technician (no probing); comparing a second unit.

## Stop conditions
No qualified person available; no restore demonstrated; any prerequisite unmet; a read that is not repeatable; a proposed step that requires circumventing access controls.
