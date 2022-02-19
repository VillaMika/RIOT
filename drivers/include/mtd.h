/*
 * Copyright (C) 2016  OTA keys S.A.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_mtd Memory Technology Device
 * @ingroup     drivers_storage
 * @{
 * @brief       Low level Memory Technology Device interface
 *
 * Generic memory technology device interface
 *
 * Unlike the @ref drivers_periph_flashpage, this is device driver based (i.e.
 * all functions take a @ref mtd_dev_t as a first argument), so that SPI based
 * EEPROMs (e.g. @ref drivers_mtd_at25xxx "AT25xxx") can be accessed the same
 * way as @ref drivers_mtd_flashpage "internal flash" or @ref
 * drivers_mtd_sdcard "SD cards"), all inside the same application.
 *
 * MTD devices expose a block based erase and write interface. In that, they
 * are the distinct from block devices (like hard disks) on which individual
 * bytes can be overwritten. The [Linux MTD FAQ](http://www.linux-mtd.infradead.org/faq/general.html)
 * has a convenient comparison (beware though of terminology differences
 * outlined below). They can be erased (with some granularity, often wearing
 * out the erased area a bit), and erased areas can be written to (sometimes
 * multiple times).
 *
 * MTD devices are described in terms of sectors, pages and feature flags:
 *
 * * A **sector** is the device's erase unit. Calls to @ref mtd_erase need to
 *   work in alignment with this number (commonly somewhere around 1kiB).
 *
 *   (Note that this corresponse to the term "page" as used in the flashpage
 *   API, and the term "eraseblock" in Linux's MTD).
 *
 * * A **page** is the largest a device can write in one transfer.
 *
 *   Applications rarely need to deal with this; it offers no guarantees on
 *   atomicity, but writing within a page is generally faster than across page
 *   boundaries.
 *
 *   Pages are a subdivision of sectors.
 *
 * * The device's **flags** indicate features, eg. whether a memory location
 *   can be overwritten without erasing it first.
 *
 * Note that some properties of the backend are currently not advertised to the
 * user (see the documentation of @ref mtd_write).
 *
 * @file
 *
 * @author      Aurelien Gonce <aurelien.gonce@altran.com>
 * @author      Vincent Dupont <vincent@otakeys.com>
 */

#ifndef MTD_H
#define MTD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   MTD power states
 */
enum mtd_power_state {
    MTD_POWER_UP,    /**< Power up */
    MTD_POWER_DOWN,  /**< Power down */
};

/**
 * @brief   MTD driver interface
 *
 * This define the functions to access a MTD.
 *
 * A MTD is composed of pages combined into sectors. A sector is the smallest erasable unit.
 * The number of pages in a sector must be constant for the whole MTD.
 *
 * The erase operation is available only for entire sectors.
 */
typedef struct mtd_desc mtd_desc_t;

/**
 * @brief   MTD device descriptor
 *
 * See the @ref drivers_mtd "module level documentation" for details on the
 * field semantics.
 */
typedef struct {
    const mtd_desc_t *driver;  /**< MTD driver */
    uint32_t sector_count;     /**< Number of sector in the MTD */
    uint32_t pages_per_sector; /**< Number of pages by sector in the MTD */
    uint32_t page_size;        /**< Size of the pages in the MTD */
#if defined(MODULE_MTD_WRITE_PAGE) || DOXYGEN
    void *work_area;           /**< sector-sized buffer (only present when @ref mtd_write_page is enabled) */
#endif
} mtd_dev_t;

/**
 * @brief   MTD driver can write any data to the storage without erasing it first.
 *
 * If this is set, a write completely overrides the previous values.
 *
 * Its absence makes no statement on whether or not writes to memory areas that
 * have been written to previously are allowed, and if so, whether previously
 * written bits should be written again or not written.
 */
#define MTD_DRIVER_FLAG_DIRECT_WRITE    (1 << 0)

/**
 * @brief   MTD driver interface
 *
 * This define the functions to access a MTD.
 *
 * A MTD is composed of pages combined into sectors. A sector is the smallest erasable unit.
 * The number of pages in a sector must be constant for the whole MTD.
 *
 * The erase operation is available only for entire sectors.
 */
struct mtd_desc {
    /**
     * @brief   Initialize Memory Technology Device (MTD)
     *
     * @param[in] dev  Pointer to the selected driver
     *
     * @returns 0 on success
     * @returns < 0 value in error
     */
    int (*init)(mtd_dev_t *dev);

    /**
     * @brief   Read from the Memory Technology Device (MTD)
     *
     * No alignment is required on @p addr and @p size.
     *
     * @param[in]  dev      Pointer to the selected driver
     * @param[out] buff     Pointer to the data buffer to store read data
     * @param[in]  addr     Starting address
     * @param[in]  size     Number of bytes
     *
     * @return 0 on success
     * @return < 0 value on error
     */
    int (*read)(mtd_dev_t *dev,
                void *buff,
                uint32_t addr,
                uint32_t size);

    /**
     * @brief   Read from the Memory Technology Device (MTD) using
     *          pagewise addressing.
     *
     * @p offset should not exceed the page size
     *
     * @param[in]  dev      Pointer to the selected driver
     * @param[out] buff     Pointer to the data buffer to store read data
     * @param[in]  page     Page number to start reading from
     * @param[in]  offset   Byte offset from the start of the page
     * @param[in]  size     Number of bytes
     *
     * @return number of bytes read on success
     * @return < 0 value on error
     */
    int (*read_page)(mtd_dev_t *dev,
                     void *buff,
                     uint32_t page,
                     uint32_t offset,
                     uint32_t size);

    /**
     * @brief   Write to the Memory Technology Device (MTD)
     *
     * @p addr + @p size must be inside a page boundary. @p addr can be anywhere
     * but the buffer cannot overlap two pages.
     *
     * @param[in] dev       Pointer to the selected driver
     * @param[in] buff      Pointer to the data to be written
     * @param[in] addr      Starting address
     * @param[in] size      Number of bytes
     *
     * @return 0 on success
     * @return < 0 value on error
     */
    int (*write)(mtd_dev_t *dev,
                 const void *buff,
                 uint32_t addr,
                 uint32_t size);

    /**
     * @brief   Write to the Memory Technology Device (MTD) using
     *          pagewise addressing.
     *
     * @p offset should not exceed the page size
     *
     * @param[in]  dev      Pointer to the selected driver
     * @param[out] buff     Pointer to the data to be written
     * @param[in]  page     Page number to start writing to
     * @param[in]  offset   Byte offset from the start of the page
     * @param[in]  size     Number of bytes
     *
     * @return bytes written on success
     * @return < 0 value on error
     */
    int (*write_page)(mtd_dev_t *dev,
                      const void *buff,
                      uint32_t page,
                      uint32_t offset,
                      uint32_t size);

    /**
     * @brief   Erase sector(s) over the Memory Technology Device (MTD)
     *
     * @p addr must be aligned on a sector boundary. @p size must be a multiple of a sector size.
     *
     * @param[in] dev       Pointer to the selected driver
     * @param[in] addr      Starting address
     * @param[in] size      Number of bytes
     *
     * @return 0 on success
     * @return < 0 value on error
     */
    int (*erase)(mtd_dev_t *dev,
                 uint32_t addr,
                 uint32_t size);

    /**
     * @brief   Erase sector(s) of the Memory Technology Device (MTD)
     *
     * @param[in] dev       Pointer to the selected driver
     * @param[in] sector    the first sector number to erase

     * @param[in] count     Number of sectors to erase
     *
     * @return 0 on success
     * @return < 0 value on error
     */
    int (*erase_sector)(mtd_dev_t *dev,
                        uint32_t sector,
                        uint32_t count);

    /**
     * @brief   Control power of Memory Technology Device (MTD)
     *
     * @param[in] dev       Pointer to the selected driver
     * @param[in] power     Power state to apply (from @ref mtd_power_state)
     *
     * @return 0 on success
     * @return < 0 value on error
     */
    int (*power)(mtd_dev_t *dev, enum mtd_power_state power);

    /**
     * @brief   Properties of the MTD driver
     */
    uint8_t flags;
};

/**
 * @brief   mtd_init Initialize a MTD device
 *
 * @param mtd the device to initialize
 *
 * @return
 */
int mtd_init(mtd_dev_t *mtd);

/**
 * @brief   Read data from a MTD device
 *
 * No alignment is required on @p addr and @p count.
 *
 * @param      mtd   the device to read from
 * @param[out] dest  the buffer to fill in
 * @param[in]  addr  the start address to read from
 * @param[in]  count the number of bytes to read
 *
 * @return 0 on success
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation is not supported on @p mtd
 * @return -EOVERFLOW if @p addr or @p count are not valid, i.e. outside memory
 * @return -EIO if I/O error occurred
 */
int mtd_read(mtd_dev_t *mtd, void *dest, uint32_t addr, uint32_t count);

/**
 * @brief   Read data from a MTD device with pagewise addressing
 *
 * The MTD layer will take care of splitting up the transaction into multiple
 * reads if it is required by the underlying storage media.
 *
 * @p offset must be smaller than the page size
 *
 * @param      mtd      the device to read from
 * @param[out] dest     the buffer to fill in
 * @param[in]  page     Page number to start reading from
 * @param[in]  offset   offset from the start of the page (in bytes)
 * @param[in]  size     the number of bytes to read
 *
 * @return 0 on success
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation is not supported on @p mtd
 * @return -EOVERFLOW if @p addr or @p count are not valid, i.e. outside memory
 * @return -EIO if I/O error occurred
 */
int mtd_read_page(mtd_dev_t *mtd, void *dest, uint32_t page, uint32_t offset, uint32_t size);

/**
 * @brief   Write data to a MTD device
 *
 * @p addr + @p count must be inside a page boundary. @p addr can be anywhere
 * but the buffer cannot overlap two pages. Though some devices might enforce alignment
 * on both @p addr and @p buf.
 *
 * @param      mtd   the device to write to
 * @param[in]  src   the buffer to write
 * @param[in]  addr  the start address to write to
 * @param[in]  count the number of bytes to write
 *
 * @return 0 on success
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation is not supported on @p mtd
 * @return -EOVERFLOW if @p addr or @p count are not valid, i.e. outside memory,
 * or overlapping two pages
 * @return -EIO if I/O error occurred
 * @return -EINVAL if parameters are invalid (invalid alignment for instance)
 */
int mtd_write(mtd_dev_t *mtd, const void *src, uint32_t addr, uint32_t count);

/**
 * @brief   Write data to a MTD device with pagewise addressing
 *
 * The MTD layer will take care of splitting up the transaction into multiple
 * writes if it is required by the underlying storage media.
 *
 * This performs a raw write, no automatic read-modify-write cycle is performed.
 *
 * @p offset must be smaller than the page size
 *
 * @param      mtd      the device to write to
 * @param[in]  src      the buffer to write
 * @param[in]  page     Page number to start writing to
 * @param[in]  offset   byte offset from the start of the page
 * @param[in]  size     the number of bytes to write
 *
 * @return 0 on success
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation is not supported on @p mtd
 * @return -EOVERFLOW if @p addr or @p count are not valid, i.e. outside memory,
 * @return -EIO if I/O error occurred
 * @return -EINVAL if parameters are invalid
 */
int mtd_write_page_raw(mtd_dev_t *mtd, const void *src, uint32_t page,
                       uint32_t offset, uint32_t size);

/**
 * @brief   Write data to a MTD device with pagewise addressing
 *
 * The MTD layer will take care of splitting up the transaction into multiple
 * writes if it is required by the underlying storage media.
 *
 * If the underlying sector needs to be erased before it can be written, the MTD
 * layer will take care of the read-modify-write operation.
 *
 * @p offset must be smaller than the page size
 *
 * @note this requires the `mtd_write_page` module
 *
 * @param      mtd      the device to write to
 * @param[in]  src      the buffer to write
 * @param[in]  page     Page number to start writing to
 * @param[in]  offset   byte offset from the start of the page
 * @param[in]  size     the number of bytes to write
 *
 * @return 0 on success
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation is not supported on @p mtd
 * @return -EOVERFLOW if @p addr or @p count are not valid, i.e. outside memory,
 * @return -EIO if I/O error occurred
 * @return -EINVAL if parameters are invalid
 */
int mtd_write_page(mtd_dev_t *mtd, const void *src, uint32_t page,
                   uint32_t offset, uint32_t size);

/**
 * @brief   Erase sectors of a MTD device
 *
 * @p addr must be aligned on a sector boundary. @p count must be a multiple of a sector size.
 *
 * @param      mtd   the device to erase
 * @param[in]  addr  the address of the first sector to erase
 * @param[in]  count the number of bytes to erase
 *
 * @return 0 if erase successful
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation is not supported on @p mtd
 * @return -EOVERFLOW if @p addr or @p count are not valid, i.e. outside memory
 * @return -EIO if I/O error occurred
 */
int mtd_erase(mtd_dev_t *mtd, uint32_t addr, uint32_t count);

/**
 * @brief   Erase sectors of a MTD device
 *
 * @param      mtd    the device to erase
 * @param[in]  sector the first sector number to erase
 * @param[in]  num    the number of sectors to erase
 *
 * @return 0 if erase successful
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation is not supported on @p mtd
 * @return -EOVERFLOW if @p addr or @p sector are not valid, i.e. outside memory
 * @return -EIO if I/O error occurred
 */
int mtd_erase_sector(mtd_dev_t *mtd, uint32_t sector, uint32_t num);

/**
 * @brief   Set power mode on a MTD device
 *
 * @param      mtd   the device to access
 * @param[in]  power the power mode to set
 *
 * @return 0 if power mode successfully set
 * @return < 0 if an error occurred
 * @return -ENODEV if @p mtd is not a valid device
 * @return -ENOTSUP if operation or @p power state is not supported on @p mtd
 * @return -EIO if I/O error occurred
 */
int mtd_power(mtd_dev_t *mtd, enum mtd_power_state power);

#ifdef __cplusplus
}
#endif

#endif /* MTD_H */
/** @} */
