/**
 * \file            DataModel.c
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

#include "DataModel.h"
#include "custom_assert.h"

/**
 * \brief           Notifies all attached observers about a data change
 * \param[in]       in_cfg: Pointer to DataModel configuration
 */
static void prv_nofify_observers(const datamodel_cfg_t* const in_cfg);

/**
 * \brief           Copies bytes from source to destination buffer
 * \param[out]      out_au8Destination: Pointer to destination buffer
 * \param[in]       in_au8Source: Pointer to source buffer
 * \param[in]       in_byte_buffer_size: Number of bytes to copy
 */
static void prv_copy_bytes(u8* out_au8Destination, const u8* in_au8Source, u32 in_byte_buffer_size);

/**
 * \brief           Checks the canary words for integrity
 * \param[in]       in_cfg: Pointer to DataModel configuration
 */
static void prv_check_canary_words(const datamodel_cfg_t* const in_cfg);

/**
 * \brief           Initialize the DataModel configuration structure
 * \param[in,out]   inout_cfg: Pointer to DataModel configuration to initialize
 */
void datamodel_init(datamodel_cfg_t* const inout_cfg)
{
    { // Input Checks
        ASSERT(inout_cfg);
        ASSERT(false == inout_cfg->is_initialized);
    }

    // Set the Canary Values
    inout_cfg->canary_word_start = DATAMODEL_CANARY_VALUE;
    inout_cfg->canary_word_end = DATAMODEL_CANARY_VALUE;

    // Reset the observer array
    for (u32 i = 0; i < DATAMODEL_MAX_OBSERVERS; i++)
    {
        inout_cfg->observers[i].notification_fn = NULL;
        inout_cfg->observers[i].observer_id = DATAMODEL_PLACEHOLDER;
    }

    // Reset the Nof Observers
    inout_cfg->nof_attached_observers = 0;

    // Clear out the static array
    for (u32 i = 0; i < DATAMODEL_MAX_CONTENT_SIZE_BYTES; i++)
    {
        inout_cfg->content_buffer[i] = 0;
    }

    // Set the validity flag
    inout_cfg->is_data_valid = false;

    // Set the initialization flag
    inout_cfg->is_initialized = true;
}

/**
 * \brief           Write data to the DataModel
 * \param[in,out]   inout_cfg: Pointer to DataModel configuration
 * \param[in]       in_byte_buffer: Pointer to data to write
 * \param[in]       in_byte_buffer_size: Size of data in bytes
 */
void datamodel_write(datamodel_cfg_t* const inout_cfg, const void* const in_byte_buffer, const u32 in_byte_buffer_size)
{
    { // Input Checks
        ASSERT(true == inout_cfg->is_initialized);
        ASSERT(in_byte_buffer);
        ASSERT(DATAMODEL_MAX_CONTENT_SIZE_BYTES > in_byte_buffer_size);
    }

    // Store the number of bytes in the DataModel
    inout_cfg->nof_stored_bytes_in_content_buffer = in_byte_buffer_size;

    // Copy the data into the content pointer
    prv_copy_bytes((u8*)inout_cfg->content_buffer, (const u8*)in_byte_buffer,
                   inout_cfg->nof_stored_bytes_in_content_buffer);

    // Notify observers about the data change
    prv_nofify_observers(inout_cfg);

    // Set validity
    datamodel_set_validity(inout_cfg, true);

    // Check Canary Words
    prv_check_canary_words(inout_cfg);
}

/**
 * \brief           Read data from the DataModel
 * \param[in]       in_cfg: Pointer to DataModel configuration
 * \param[out]      out_byte_buffer: Pointer to buffer to store read data
 * \param[out]      out_byte_buffer_size: Pointer to store size of read data
 */
void datamodel_read(const datamodel_cfg_t* const in_cfg, void* const out_byte_buffer, u32* const out_byte_buffer_size)
{
    { // Input Checks
        ASSERT(in_cfg);
        ASSERT(in_cfg->is_initialized);
        ASSERT(in_cfg->content_buffer);
        ASSERT(out_byte_buffer);
        ASSERT(out_byte_buffer_size);
        ASSERT(DATAMODEL_MAX_CONTENT_SIZE_BYTES > in_cfg->nof_stored_bytes_in_content_buffer);
    }

    // Copy the bytes from the stored content into the output buffer
    prv_copy_bytes((u8*)out_byte_buffer, (const u8*)in_cfg->content_buffer, in_cfg->nof_stored_bytes_in_content_buffer);

    *out_byte_buffer_size = in_cfg->nof_stored_bytes_in_content_buffer;

    // Check Canary Words
    prv_check_canary_words(in_cfg);
}

/**
 * \brief           Check if the DataModel contains valid data
 * \param[in]       in_cfg: Pointer to DataModel configuration
 * \return          `true` if data is valid, `false` otherwise
 */
bool datamodel_is_valid(const datamodel_cfg_t* const in_cfg)
{
    { // Input Checks
        ASSERT(in_cfg);
        ASSERT(in_cfg->is_initialized);
    }

    // Check Canary Words
    prv_check_canary_words(in_cfg);
    return in_cfg->is_data_valid;
}

/**
 * \brief           Set the validity flag of the DataModel
 * \param[in,out]   inout_cfg: Pointer to DataModel configuration
 * \param[in]       in_is_valid: Validity flag to set
 */
void datamodel_set_validity(datamodel_cfg_t* const inout_cfg, bool in_is_valid)
{
    { // Input Checks
        ASSERT(inout_cfg);
        ASSERT(inout_cfg->is_initialized);
    }

    inout_cfg->is_data_valid = in_is_valid;

    // Check Canary Words
    prv_check_canary_words(inout_cfg);
}

/**
 * \brief           Attach an observer to the DataModel
 * \param[in,out]   inout_cfg: Pointer to DataModel configuration
 * \param[in]       in_observer: Pointer to observer to attach
 */
void datamodel_attach_observer(datamodel_cfg_t* const inout_cfg, const datamodel_observer_t* const in_observer)
{
    { // Input Checks
        ASSERT(inout_cfg);
        ASSERT(inout_cfg->is_initialized);
        ASSERT(inout_cfg->observers);

        ASSERT(DATAMODEL_PLACEHOLDER != in_observer->observer_id);
        ASSERT(in_observer->notification_fn);
    }

    // Check if there is space for a new observer
    bool bFoundSpace = false;
    bool bIsAlreadyAttached = false;
    for (u32 i = 0; i < DATAMODEL_MAX_OBSERVERS; i++)
    {

        // Check for duplicate observers
        if (in_observer->observer_id == inout_cfg->observers[i].observer_id)
        {
            bIsAlreadyAttached = true;
        }

        // Look for an empty space
        if ((NULL == inout_cfg->observers[i].notification_fn)
            && (DATAMODEL_PLACEHOLDER == inout_cfg->observers[i].observer_id))
        {
            inout_cfg->observers[i].observer_id = in_observer->observer_id;
            inout_cfg->observers[i].notification_fn = in_observer->notification_fn;
            bFoundSpace = true;
        }
    }

    ASSERT(true == bFoundSpace);
    ASSERT(false == bIsAlreadyAttached);

    inout_cfg->nof_attached_observers++;
    ASSERT(inout_cfg->nof_attached_observers <= DATAMODEL_MAX_OBSERVERS);

    // Check Canary Words
    prv_check_canary_words(inout_cfg);
}

/**
 * \brief           Detach an observer from the DataModel
 * \param[in,out]   inout_cfg: Pointer to DataModel configuration
 * \param[in]       in_observer: Pointer to observer to detach
 */
void datamodel_detach_observer(datamodel_cfg_t* const inout_cfg, const datamodel_observer_t* const in_observer)
{
    { // Input Checks
        ASSERT(inout_cfg);
        ASSERT(inout_cfg->is_initialized);
        ASSERT(inout_cfg->observers);

        ASSERT(DATAMODEL_PLACEHOLDER != in_observer->observer_id);
    }

    // Search for the observer and remove it
    bool bFoundObserver = false;
    for (u32 i = 0; i < DATAMODEL_MAX_OBSERVERS; i++)
    {
        if (inout_cfg->observers[i].observer_id == in_observer->observer_id)
        {
            inout_cfg->observers[i].observer_id = DATAMODEL_PLACEHOLDER;
            inout_cfg->observers[i].notification_fn = NULL;
            bFoundObserver = true;
            break;
        }
    }

    ASSERT(true == bFoundObserver);
    inout_cfg->nof_attached_observers--;
    ASSERT(inout_cfg->nof_attached_observers < DATAMODEL_MAX_OBSERVERS);

    // Check Canary Words
    prv_check_canary_words(inout_cfg);
}

void prv_nofify_observers(const datamodel_cfg_t* const in_cfg)
{
    { // Input Checks
        ASSERT(in_cfg);
        ASSERT(in_cfg->is_initialized);
        ASSERT(in_cfg->observers);
    }

    bool bOneObserverIsNotified = false;
    for (u32 i = 0; i < DATAMODEL_MAX_OBSERVERS; i++)
    {
        if ((in_cfg->observers[i].notification_fn) && (DATAMODEL_PLACEHOLDER != in_cfg->observers[i].observer_id))
        {
            in_cfg->observers[i].notification_fn();
            bOneObserverIsNotified = true;
        }
    }
    ASSERT(true == bOneObserverIsNotified);
}

void prv_check_canary_words(const datamodel_cfg_t* const in_cfg)
{
    { // Input Checks
        ASSERT(in_cfg);
        ASSERT(in_cfg->is_initialized);
        ASSERT(DATAMODEL_CANARY_VALUE == in_cfg->canary_word_start);
        ASSERT(DATAMODEL_CANARY_VALUE == in_cfg->canary_word_end);
    }
}

/**
 * \brief           Get the number of attached observers
 * \param[in]       in_cfg: Pointer to DataModel configuration
 * \return          Number of attached observers
 */
u32 datamodel_get_nof_attached_observers(const datamodel_cfg_t* const in_cfg)
{
    { // Input Checks
        ASSERT(in_cfg);
        ASSERT(in_cfg->is_initialized);
        ASSERT(in_cfg->observers);
        ASSERT(in_cfg->nof_attached_observers <= DATAMODEL_MAX_OBSERVERS);
    }
    return in_cfg->nof_attached_observers;
}

void prv_copy_bytes(u8* out_au8Destination, const u8* in_au8Source, u32 in_byte_buffer_size)
{
    { // Input Checks
        ASSERT(out_au8Destination);
        ASSERT(in_au8Source);
        ASSERT(in_byte_buffer_size > 0);
    }
    for (u32 i = 0; i < in_byte_buffer_size; ++i)
    {
        out_au8Destination[i] = in_au8Source[i];
    }
}
