
/**
 * \file            DataModel.h
 * \brief           Data model implementation for embedded systems
 */

/*
 * Copyright (c) 2025 Max Koell
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:          Max Koell <maxkoell@proton.me>
 */

#ifndef DATAMODEL_HDR_H
#define DATAMODEL_HDR_H

#include "custom_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define DATAMODEL_MAX_CONTENT_SIZE_BYTES (10U)
#define DATAMODEL_MAX_OBSERVERS          (5U)
#define DATAMODEL_CANARY_VALUE           (0xA5A5A5A5U)
#define DATAMODEL_PLACEHOLDER            (0xFFFFFFFFU)

    /**
 * @brief Observer structure for notification callbacks.
 */
    typedef struct
    {
        /** @brief Unique identifier for the observer. */
        u32 observer_id;

        /** @brief Pointer to the notification callback function. */
        void (*notification_fn)(void);
    } datamodel_observer_t;

    /**
 * @brief Data model structure for storing content and observers.
 */
    typedef struct
    {
        /** @brief Start canary word for integrity check. */
        u32 canary_word_start;

        /** @brief Array of attached observers. */
        datamodel_observer_t observers[DATAMODEL_MAX_OBSERVERS];

        /** @brief Number of attached observers. */
        u32 nof_attached_observers;

        /** @brief Content buffer. */
        u8 content_buffer[DATAMODEL_MAX_CONTENT_SIZE_BYTES];

        /** @brief number of stored bytes */
        u32 nof_stored_bytes_in_content_buffer;

        /** @brief Data validity flag. */
        bool is_data_valid;

        /** @brief Initialization flag. */
        bool is_initialized;

        /** @brief End canary word for integrity check. */
        u32 canary_word_end;
    } datamodel_cfg_t;

    void datamodel_init(datamodel_cfg_t* const inout_cfg);

    void datamodel_write(datamodel_cfg_t* const inout_cfg, const void* const in_data_buffer, const u32 in_data_size);

    void datamodel_read(const datamodel_cfg_t* const in_ptDataModel, void* const out_ptDataBytes,
                        u32* const out_ptDataSize);

    bool datamodel_is_valid(const datamodel_cfg_t* const in_ptDataModel);

    void datamodel_set_validity(datamodel_cfg_t* const inout_ptDataModel, bool bIsValid);

    void datamodel_attach_observer(datamodel_cfg_t* const inout_ptDataModel,
                                   const datamodel_observer_t* const in_ptObserver);

    void datamodel_detach_observer(datamodel_cfg_t* const inout_ptDataModel,
                                   const datamodel_observer_t* const in_ptObserver);

    u32 datamodel_get_nof_attached_observers(const datamodel_cfg_t* const in_ptDataModel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DATAMODEL_HDR_H */
