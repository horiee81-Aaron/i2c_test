#include "hal_i2c.h"
#include "mock_hal_scheduler.h"
#include "mock_r_config_iica0.h"
#include "r_config_iica0.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_RECORDED_ERRORS (32U)

static hal_i2c_error_context_t g_recorded_errors[MAX_RECORDED_ERRORS];
static uint32_t g_recorded_error_count = 0U;
static bool g_error_overflow = false;
static uint32_t g_failed_asserts = 0U;
static uint32_t g_total_asserts = 0U;

static void test_error_callback(const hal_i2c_error_context_t *context)
{
    if (context != NULL)
    {
        if (g_recorded_error_count < MAX_RECORDED_ERRORS)
        {
            g_recorded_errors[g_recorded_error_count] = *context;
        }
        else
        {
            g_error_overflow = true;
        }
    }

    g_recorded_error_count++;
}

static void reset_test_observers(void)
{
    memset(g_recorded_errors, 0, sizeof g_recorded_errors);
    g_recorded_error_count = 0U;
    g_error_overflow = false;
}

static void test_setup(void)
{
    reset_test_observers();
    MOCK_R_Config_IICA0_Reset();
    MOCK_HAL_SCHED_Reset();
    HAL_I2C_Init(test_error_callback);
}

#define TEST_ASSERT(expr)                                                                 \
    do                                                                                    \
    {                                                                                     \
        g_total_asserts++;                                                                \
        if (!(expr))                                                                      \
        {                                                                                 \
            g_failed_asserts++;                                                           \
            printf("    Assertion failed: %s (line %u)\n", #expr, (unsigned)__LINE__);    \
            return;                                                                       \
        }                                                                                 \
    } while (0)

static void test_init_rearms_hardware(void)
{
    test_setup();
    const mock_r_config_iica0_state_t *state = MOCK_R_Config_IICA0_GetState();
    TEST_ASSERT(state->stop_calls == 1U);
    TEST_ASSERT(state->create_calls == 1U);
    TEST_ASSERT(state->start_calls == 1U);
    TEST_ASSERT(state->slave_receive_start_calls == 1U);
    TEST_ASSERT(g_recorded_error_count == 0U);
    TEST_ASSERT(g_error_overflow == false);
    hal_i2c_message_t message;
    TEST_ASSERT(HAL_I2C_PopMessage(&message) == false);
}

static void test_message_buffering_and_pop(void)
{
    test_setup();
    MOCK_HAL_SCHED_SetUptime(100U);
    HAL_I2C_OnStartCondition(0x11U);
    HAL_I2C_OnByteReceived(0xA0U);
    HAL_I2C_OnByteReceived(0x0FU);
    MOCK_HAL_SCHED_SetUptime(250U);
    HAL_I2C_OnStopCondition(0x05U);

    hal_i2c_message_t message;
    TEST_ASSERT(HAL_I2C_PopMessage(&message) == true);
    TEST_ASSERT(message.length == 2U);
    TEST_ASSERT(message.data[0] == 0xA0U);
    TEST_ASSERT(message.data[1] == 0x0FU);
    TEST_ASSERT(message.hw_status_flags == 0x05U);
    TEST_ASSERT(message.timestamp_ms == 250U);
    TEST_ASSERT(g_recorded_error_count == 0U);
}

static void test_slave_response_set_get_clear(void)
{
    test_setup();
    const uint8_t response[] = { 0x10U, 0x20U, 0x30U };

    TEST_ASSERT(HAL_I2C_SetSlaveResponse(response, (uint8_t)sizeof response) == true);

    const uint8_t *payload = NULL;
    uint8_t length = 0U;
    TEST_ASSERT(HAL_I2C_GetSlaveResponse(&payload, &length) == true);
    TEST_ASSERT(length == (uint8_t)sizeof response);
    TEST_ASSERT(payload != NULL);
    TEST_ASSERT(memcmp(payload, response, sizeof response) == 0);

    HAL_I2C_ClearSlaveResponse();
    payload = (const uint8_t *)0x1U;
    length = 255U;
    TEST_ASSERT(HAL_I2C_GetSlaveResponse(&payload, &length) == false);
    TEST_ASSERT(payload == NULL);
    TEST_ASSERT(length == 0U);
}

static void test_slave_response_rejects_invalid_length(void)
{
    test_setup();
    const uint8_t response[HAL_I2C_MESSAGE_MAX_BYTES + 1U] = {0};
    TEST_ASSERT(HAL_I2C_SetSlaveResponse(response, (uint8_t)(HAL_I2C_MESSAGE_MAX_BYTES + 1U)) == false);
    const uint8_t *payload = (const uint8_t *)0x2U;
    uint8_t length = 123U;
    TEST_ASSERT(HAL_I2C_GetSlaveResponse(&payload, &length) == false);
    TEST_ASSERT(payload == NULL);
    TEST_ASSERT(length == 0U);
}

static void test_overrun_on_long_message_triggers_reset(void)
{
    test_setup();
    MOCK_HAL_SCHED_SetUptime(10U);
    HAL_I2C_OnStartCondition(0x00U);

    for (uint8_t index = 0U; index < HAL_I2C_MESSAGE_MAX_BYTES; index++)
    {
        HAL_I2C_OnByteReceived(index);
    }

    const mock_r_config_iica0_state_t before = *MOCK_R_Config_IICA0_GetState();
    HAL_I2C_OnByteReceived(0xFFU);
    const mock_r_config_iica0_state_t after = *MOCK_R_Config_IICA0_GetState();

    TEST_ASSERT(after.stop_calls == (before.stop_calls + 1U));
    TEST_ASSERT(after.create_calls == (before.create_calls + 1U));
    TEST_ASSERT(after.start_calls == (before.start_calls + 1U));
    TEST_ASSERT(after.slave_receive_start_calls == (before.slave_receive_start_calls + 1U));
    TEST_ASSERT(g_recorded_error_count == 1U);
    TEST_ASSERT(g_recorded_errors[0].code == HAL_I2C_ERR_OVERRUN);
    TEST_ASSERT(g_recorded_errors[0].message_dropped == true);
}

static void test_timeout_during_reception(void)
{
    test_setup();
    HAL_I2C_OnStartCondition(0x00U);
    HAL_I2C_OnByteReceived(0x01U);

    for (uint8_t tick = 0U; tick < HAL_I2C_SLAVE_TIMEOUT_MS; tick++)
    {
        MOCK_HAL_SCHED_Advance(1U);
        HAL_I2C_Tick1ms();
    }

    TEST_ASSERT(g_recorded_error_count == 1U);
    TEST_ASSERT(g_recorded_errors[0].code == HAL_I2C_ERR_TIMEOUT);
    TEST_ASSERT(g_recorded_errors[0].message_dropped == true);
}

static void test_hardware_error_mapping(void)
{
    test_setup();

    HAL_I2C_OnHardwareError(R_IICA0_STATUS_BUS_ERROR);
    HAL_I2C_OnHardwareError(R_IICA0_STATUS_ARBITRATION_LOST);
    HAL_I2C_OnHardwareError(R_IICA0_STATUS_OVERRUN);
    HAL_I2C_OnHardwareError(R_IICA0_STATUS_NACK);
    HAL_I2C_OnHardwareError(R_IICA0_STATUS_LINE_STUCK);
    HAL_I2C_OnHardwareError(R_IICA0_STATUS_FRAME_ERROR);

    TEST_ASSERT(g_recorded_error_count == 6U);
    TEST_ASSERT(g_recorded_errors[0].code == HAL_I2C_ERR_BUS_ERROR);
    TEST_ASSERT(g_recorded_errors[1].code == HAL_I2C_ERR_ARBITRATION_LOST);
    TEST_ASSERT(g_recorded_errors[2].code == HAL_I2C_ERR_OVERRUN);
    TEST_ASSERT(g_recorded_errors[3].code == HAL_I2C_ERR_NACK);
    TEST_ASSERT(g_recorded_errors[4].code == HAL_I2C_ERR_LINE_STUCK);
    TEST_ASSERT(g_recorded_errors[5].code == HAL_I2C_ERR_FRAME);
}

static void test_ring_buffer_overflow_reports_error(void)
{
    test_setup();

    for (uint8_t count = 0U; count < HAL_I2C_RING_CAPACITY; count++)
    {
        HAL_I2C_OnStartCondition(0x00U);
        HAL_I2C_OnByteReceived((uint8_t)(0x10U + count));
        HAL_I2C_OnStopCondition(0x00U);
    }

    HAL_I2C_OnStartCondition(0x00U);
    HAL_I2C_OnByteReceived(0xAAU);
    HAL_I2C_OnStopCondition(0x77U);

    TEST_ASSERT(g_recorded_error_count == 1U);
    TEST_ASSERT(g_recorded_errors[0].code == HAL_I2C_ERR_OVERRUN);
    TEST_ASSERT(g_recorded_errors[0].message_dropped == true);
    TEST_ASSERT(g_recorded_errors[0].hw_status_flags == 0x77U);

    hal_i2c_message_t message;
    uint8_t drained = 0U;
    while (HAL_I2C_PopMessage(&message))
    {
        drained++;
    }

    TEST_ASSERT(drained == HAL_I2C_RING_CAPACITY);
}

typedef void (*test_fn_t)(void);

typedef struct
{
    const char *name;
    test_fn_t   function;
} test_case_t;

static test_case_t g_tests[] = {
    { "init_rearms_hardware", test_init_rearms_hardware },
    { "message_buffering_and_pop", test_message_buffering_and_pop },
    { "slave_response_set_get_clear", test_slave_response_set_get_clear },
    { "slave_response_rejects_invalid_length", test_slave_response_rejects_invalid_length },
    { "overrun_on_long_message_triggers_reset", test_overrun_on_long_message_triggers_reset },
    { "timeout_during_reception", test_timeout_during_reception },
    { "hardware_error_mapping", test_hardware_error_mapping },
    { "ring_buffer_overflow_reports_error", test_ring_buffer_overflow_reports_error }
};

int main(void)
{
    const size_t total_tests = sizeof g_tests / sizeof g_tests[0];
    size_t passed_tests = 0U;

    for (size_t index = 0U; index < total_tests; index++)
    {
        printf("[ RUN      ] %s\n", g_tests[index].name);
        const uint32_t failed_before = g_failed_asserts;
        g_tests[index].function();
        if (g_failed_asserts == failed_before)
        {
            printf("[     PASS ] %s\n", g_tests[index].name);
            passed_tests++;
        }
        else
        {
            printf("[   FAILED ] %s\n", g_tests[index].name);
        }
    }

    printf("[ SUMMARY  ] %zu / %zu tests passed (%u assertions)\n",
           passed_tests, total_tests, (unsigned)g_total_asserts);

    return (g_failed_asserts == 0U) ? 0 : 1;
}