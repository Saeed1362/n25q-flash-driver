# N25Q SPI Flash Driver

A C driver for Micron N25Q serial NOR flash, with a GPIO-based, bit-banged SPI implementation and a platform port layer.

The repository contains the driver and standalone hardware tests. The supplied `spi_port.c` is a starting point for STM32 integration; adapt it for your board or another microcontroller/SoC.

## Features

- JEDEC identification read
- Status and flag-status register access
- Readiness polling with timeouts
- Write enable and error-flag clearing
- Single-byte memory reads
- Page programming
- Buffer writes split automatically at page boundaries
- Subsector, sector, and bulk erase
- Device geometry information
- Hardware tests with readback comparisons

## Repository layout

| Directory | Contents |
| --- | --- |
| `include/` | `n25q.h`, `spi.h`, `spi_port.h`, and `status.h` |
| `src/` | `n25q.c`, `spi.c`, and `spi_port.c` |
| `tests/` | `n25q_test.c` and `n25q_test.h` |

Device-selection and geometry macros belong in the driver configuration section of `n25q.h`, or in a separate `n25q_config.h` if you choose to split configuration from the public API.

## Current scope

The current implementation uses single-lane SPI mode 0 and three address bytes. It can address up to 16 MiB, with addresses from `0x000000` through `0xFFFFFF`.

Changing the capacity macro alone does not implement extended addressing. Larger devices require additional driver changes before their full capacity can be accessed. Quad mode, memory-mapped access, and Fast Read are not implemented by this version.

Confirm the complete flash part number, voltage, command set, and erase geometry against its datasheet. A list of selectable densities is not a claim that every device variant has been tested.

## Hardware connection

| MCU/SoC signal | Flash signal | MCU/SoC direction |
| --- | --- | --- |
| MOSI | DQ0 | Output |
| MISO | DQ1 | Input |
| CLK | C / SCK | Output |
| CS | S# | Output, active low |
| Supply | VCC | Appropriate supply for the exact part |
| Ground | VSS | Common ground |

Configure MOSI, CLK, and CS as ordinary GPIO outputs for bit banging, rather than SPI alternate-function pins. Configure MISO as a GPIO input. Use suitable output drive settings for the board and clock speed.

Connect and bias W#, HOLD#, or RESET# as required by the exact device and board design. Do not leave active control inputs floating. Check voltage compatibility; N25Q variants are not all powered at the same voltage.

## Integration

1. Copy `include/` and `src/` into your firmware project.
2. Add `include/` to the compiler's header search paths.
3. Add `n25q.c`, `spi.c`, and `spi_port.c` to the build.
4. Select the flash variant and configure its capacity and geometry.
5. Implement or adapt the GPIO, delay, and time functions in `spi_port.c`.
6. Initialize the target clocks, GPIOs, delay timer, and time source before calling the driver.
7. Call `n25q_init()` and check its result before accessing memory.

Use a C99-or-later compiler. The public headers should include the standard headers they need, such as `<stdint.h>` and `<stdbool.h>`. Keep platform headers and HAL dependencies in the port implementation wherever possible.

The application does not need to use the same IDE as the supplied port. Any toolchain that builds the C sources and provides the port functions can integrate the driver.

## Making another MCU/SoC compatible

The flash command logic belongs in `n25q.c`. The bit-level SPI algorithm belongs in `spi.c`. Hardware-specific operations belong in `spi_port.c`, with their declarations in `spi_port.h`.

Adapt the existing port functions without changing their declarations:

| Operation | Requirement |
| --- | --- |
| `write_bit(value, pin)` | Drive the selected output GPIO low or high |
| `read_bit(pin)` | Return the selected input level as 0 or 1 |
| Microsecond delay | Provide the delay used between SPI signal transitions |
| `n25q_port_get_time_ms()` | Return an advancing unsigned 32-bit millisecond counter |

Map each logical pin to its actual GPIO port and pin. Replace STM32 HAL calls with your target's GPIO and timer APIs. Also replace any target-specific initialization or logging dependencies in the tests.

For mode 0, CLK is low when idle. MOSI must be stable before the rising edge; MISO is sampled during the high phase. Each byte is sent most significant bit first. The SPI transfer function must not change CS: the flash driver controls CS across the complete command transaction.

The existing transfer convention is:

```c
int spi_transfer(uint8_t *rx, const uint8_t *tx);
```

It exchanges one byte, returns zero on success, and reports an error with a nonzero return value. A completed bit-banged transfer does not by itself prove that the flash is connected or responded correctly.

### The 1 microsecond delay

The SPI implementation uses a short delay between GPIO transitions. A 1 microsecond delay is a starting point for a slow bring-up clock; it must be implemented and verified for the target MCU/SoC.

There is no portable C instruction that guarantees a 1 microsecond delay on every processor. CPU frequency, compiler optimization, GPIO access time, and interrupts all affect timing.

A hardware timer or a supported processor cycle counter is preferable to an uncalibrated empty loop. For a timer-based implementation:

1. Determine the actual timer input clock.
2. Configure the timer to increment at a known rate, such as 1 MHz.
3. Start it as a free-running counter.
4. Wait for the required elapsed ticks, accounting for its counter width and rollover.

A 1 MHz timer increments once per microsecond; it should not reset every microsecond. A function-call delay based on that timer has quantization and execution overhead. If a guaranteed minimum 1 microsecond interval is required, account for those effects or use a higher-resolution counter.

Do not use a millisecond delay function as a replacement for the microsecond delay. Recheck the implementation whenever the clock configuration changes. On a cached or operating-system-based SoC, GPIO and scheduling behavior may make userspace bit banging unsuitable; a hardware SPI implementation is usually preferable.

The resulting SCK frequency depends on all GPIO operations and delay calls in the bit loop. A 1 microsecond delay does not automatically mean a 1 MHz SPI clock. Verify SCK and data timing with an oscilloscope or logic analyzer and keep them within the flash datasheet limits.

### Millisecond time source

Timeouts use:

```c
uint32_t n25q_port_get_time_ms(void);
```

For STM32, this can wrap `HAL_GetTick()`. On another target, use an appropriate timer or monotonic system clock converted to milliseconds.

The counter must continue advancing while the driver waits. Unsigned subtraction handles rollover when the elapsed interval stays within one counter cycle and the timeout is checked frequently enough. Do not use a time source that stops because its tick interrupt is disabled during a driver call.

The current blocking APIs pass the timeout to individual helper operations. The timeout is not a single total deadline for the entire write or erase function; a multipage write can take longer than one timeout interval.

## Flash configuration

For an N25Q128 configuration with the corresponding uniform erase geometry:

```c
#define N25Q_CAPACITY_BYTES  16777216U
#define N25Q_PAGE_SIZE       256U
#define N25Q_SUBSECTOR_SIZE  4096U
#define N25Q_SECTOR_SIZE     65536U
```

Capacity is expressed in bytes. N25Q128 denotes 128 Mbit, which is 16 MiB. The 256-byte program page and 4096-byte erase subsector are different units.

Validate configuration against the exact part. `n25q_get_info()` returns configured geometry; it does not independently discover the device. Initialization should verify that the connected device matches a supported configuration.

## API overview

| Function | Purpose |
| --- | --- |
| `n25q_init()` | Initialize driver state and validate device identification |
| `n25q_read_id()` | Read the three JEDEC ID bytes |
| `n25q_read_register()` | Read a supported one-byte register using its read opcode |
| `n25q_wait_ready()` | Poll until the flash is ready or a timeout occurs |
| `n25q_write_enable()` | Issue Write Enable and verify WEL |
| `n25q_clear_flag_status()` | Clear stored operation-error flags |
| `n25q_read()` | Read one memory byte |
| `n25q_page_program()` | Program data contained within one page |
| `n25q_write()` | Split a buffer across page-program operations |
| `n25q_erase_subsector()` | Erase an aligned subsector |
| `n25q_erase_sector()` | Erase an aligned sector |
| `n25q_erase_chip()` | Start bulk erase and wait for completion |
| `n25q_get_info()` | Return configured flash geometry |

The declarations in `n25q.h` are authoritative. If `n25q_read_status()` is retained as a wrapper, it reads the normal status register rather than the flag-status register.

Functions return `OK` or `ERR` through the driver's `STATUS` type. Check every return value. On a failed multipage write, earlier pages may already have been programmed. A timeout does not necessarily stop the flash's internal operation.

## Example: erase, write, and read back

This example destroys the contents of the first subsector. It assumes the port and target peripherals are already initialized.

```c
#include "n25q.h"

STATUS flash_example(void)
{
    const uint8_t tx[] = {0x12U, 0x34U, 0x56U, 0x78U};
    uint8_t rx;
    const uint32_t timeout_ms = 10000U; /* Example, not a universal limit */

    if (n25q_init() != OK)
    {
        return ERR;
    }

    if (n25q_erase_subsector(0U, timeout_ms) != OK)
    {
        return ERR;
    }

    if (n25q_write(0U, tx, sizeof(tx), timeout_ms) != OK)
    {
        return ERR;
    }

    for (uint32_t i = 0U; i < sizeof(tx); i++)
    {
        if (n25q_read(i, &rx) != OK || rx != tx[i])
        {
            return ERR;
        }
    }

    return OK;
}
```

Choose operation timeouts using the device's maximum timing specifications, including the relevant voltage and temperature conditions. Bulk erase needs a much larger allowance than page programming.

## Programming and erase rules

- Programming changes bits from 1 to 0. Returning them to 1 requires an erase.
- `n25q_write()` does not erase memory automatically.
- `n25q_page_program()` accepts only data that fits within the remaining space in the current page.
- `n25q_write()` splits larger or unaligned buffers into page-sized chunks.
- Erase APIs require an address aligned to their erase-unit size.
- Erasing destroys the complete selected unit, including neighboring data inside that unit.
- Bulk erase destroys the entire chip.
- Error flags should be handled and cleared before a new program or erase, then checked after completion.
- Readiness alone is not proof of successful programming or erasing. Check operation-error flags and use readback comparisons when verification is required.

Calls are blocking. The driver does not provide thread synchronization. Serialize access to the flash and protect complete command sequences when used with an RTOS or shared bus. Do not invoke long blocking operations from an interrupt handler.

## Hardware tests

Add `tests/n25q_test.c` to the application build and add `tests/` to its include paths. Supply the logging function used by the test file, such as `iprint()`, or redirect `printf()` if that is what the test uses.

Run the tests once after hardware initialization:

```c
#include "n25q_test.h"

STATUS result = n25q_test();
```

Tests exercise initialization, byte programming, page programming, writes across page boundaries, argument validation, and erase operations. The standard test uses the first sector as a destructive scratch region. Do not run it on a device containing data you need to preserve, and do not place it in a continuously repeating main loop.

Bulk testing is optional:

```c
#define N25Q_TEST_ENABLE_BULK 0U /* Change to 1U to enable */
```

The quick bulk test programs and verifies 0x00 at the first, middle, and last byte, issues bulk erase, then checks that those locations contain 0xFF. It is a spot check, not proof that every byte was erased. Full verification requires reading the entire chip and is much slower, especially with one-byte reads and bit-banged SPI.

A passing test describes the tested hardware and software combination. Record the board, complete flash part number, supply voltage, compiler settings, and test results before describing a configuration as validated.

## Troubleshooting

| Symptom | Checks |
| --- | --- |
| JEDEC ID is FF FF FF | Check power, ground, MISO mapping, CS, control pins, and whether the flash is responding |
| JEDEC ID is 00 00 00 | Check for a low/stuck MISO signal, wiring faults, and pin configuration |
| WEL does not become set | Check readiness and that CS rises after Write Enable |
| Writes fail at page boundaries | Confirm a 256-byte page configuration and correct write chunking |
| Program or erase returns ERR | Inspect flag status, protection settings, previous error flags, and timeout selection |
| Timeout never expires | Check that the millisecond time source is advancing |
| SPI communication is unreliable | Inspect mode-0 timing, GPIO mapping, delay implementation, and signal integrity |
| Tests take a long time | Distinguish internal erase time from exhaustive readback; use bulk spot checks for routine tests |

