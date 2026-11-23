
/************************************************************
 * Project : StretchSense_BLE_Test
 * MCU     : Nordic nRF52832
 * SDK     : nRF Connect SDK v3.3.5
 *
 * Function:
 * 1. Initialize Bluetooth Low Energy (BLE)
 * 2. Advertise the device as a BLE Peripheral
 * 3. Create a custom GATT Service
 * 4. Generate two channels of simulated sensor data
 * 5. Transmit data through BLE Notifications
 * 6. Automatically restart advertising after disconnection
 *
 * Data transmission:
 * CH1: uint16_t
 * CH2: uint16_t
 * Sampling interval: 100 ms
 * Transmission rate: 10 packets/s
 ************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/************************************************************
 * 1. Device configuration
 ************************************************************/

#define DEVICE_NAME "StretchSense"

#define BT_UUID_SENSOR_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, \
                       0x1234, 0x56789abcdef0)

#define BT_UUID_SENSOR_DATA_VAL \
    BT_UUID_128_ENCODE(0x12345679, 0x1234, 0x5678, \
                       0x1234, 0x56789abcdef0)

#define BT_UUID_SENSOR_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_SENSOR_SERVICE_VAL)

#define BT_UUID_SENSOR_DATA \
    BT_UUID_DECLARE_128(BT_UUID_SENSOR_DATA_VAL)

/************************************************************
 * 2. Global variables
 ************************************************************/

static bool send_enabled = false;
static bool ble_connected = false;

static uint16_t ch1 = 1000;
static uint16_t ch2 = 2000;

/************************************************************
 * 3. Advertising data
 ************************************************************/

static const struct bt_data advertising_data[] = {
    BT_DATA_BYTES(
        BT_DATA_FLAGS,
        BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR
    ),
    BT_DATA(
        BT_DATA_NAME_COMPLETE,
        DEVICE_NAME,
        sizeof(DEVICE_NAME) - 1
    ),
};

/************************************************************
 * 4. Advertising restart work
 ************************************************************/

static void advertising_start_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(
    advertising_start_work,
    advertising_start_handler
);

static void advertising_start_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    if (ble_connected) {
        return;
    }

    int err = bt_le_adv_start(
        BT_LE_ADV_CONN_FAST_1,
        advertising_data,
        ARRAY_SIZE(advertising_data),
        NULL,
        0
    );

    if (err == 0) {
        printk("Advertising started successfully.\n");
    } else {
        printk("Advertising start failed: %d. Retrying...\n", err);

        k_work_schedule(
            &advertising_start_work,
            K_SECONDS(1)
        );
    }
}

/************************************************************
 * 5. GATT CCC callback
 ************************************************************/

static void ccc_changed(
    const struct bt_gatt_attr *attr,
    uint16_t value
)
{
    ARG_UNUSED(attr);

    if (value == BT_GATT_CCC_NOTIFY) {
        send_enabled = true;
        printk("Notification enabled.\n");
    } else {
        send_enabled = false;
        printk("Notification disabled.\n");
    }
}

/************************************************************
 * 6. Custom GATT Service
 ************************************************************/

BT_GATT_SERVICE_DEFINE(
    sensor_service,

    BT_GATT_PRIMARY_SERVICE(BT_UUID_SENSOR_SERVICE),

    BT_GATT_CHARACTERISTIC(
        BT_UUID_SENSOR_DATA,
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL,
        NULL,
        NULL
    ),

    BT_GATT_CCC(
        ccc_changed,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE
    )
);

/************************************************************
 * 7. BLE connection callbacks
 ************************************************************/

static void connected(struct bt_conn *conn, uint8_t err)
{
    ARG_UNUSED(conn);

    if (err) {
        printk("Connection failed: %u\n", err);
        return;
    }

    ble_connected = true;

    k_work_cancel_delayable(&advertising_start_work);

    send_enabled = false;

    printk("BLE connected.\n");
}

static void disconnected(
    struct bt_conn *conn,
    uint8_t reason
)
{
    ARG_UNUSED(conn);

    ble_connected = false;
    send_enabled = false;

    printk("BLE disconnected: %u\n", reason);

    /*
     * Restart advertising outside the connection callback.
     * A short delay allows the BLE connection state to settle.
     */

    k_work_schedule(
        &advertising_start_work,
        K_MSEC(500)
    );
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/************************************************************
 * 8. Sensor data generation and transmission
 ************************************************************/

static void sensor_work_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(
    sensor_work,
    sensor_work_handler
);

static void sensor_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    ch1 += 7;
    ch2 += 11;

    if (ch1 > 3000) {
        ch1 = 1000;
    }

    if (ch2 > 3500) {
        ch2 = 1500;
    }

    uint8_t data[4];

    sys_put_le16(ch1, &data[0]);
    sys_put_le16(ch2, &data[2]);

    if (send_enabled && ble_connected) {

        int err = bt_gatt_notify(
            NULL,
            &sensor_service.attrs[2],
            data,
            sizeof(data)
        );

        if (err) {
            printk("Notification error: %d\n", err);
        }
    }

    k_work_schedule(
        &sensor_work,
        K_MSEC(100)
    );
}

/************************************************************
 * 9. Main function
 ************************************************************/

int main(void)
{
    int err;

    printk("\n");
    printk("================================\n");
    printk("     StretchSense BLE Sensor\n");
    printk("================================\n");

    err = bt_enable(NULL);

    if (err) {
        printk("Bluetooth initialization failed: %d\n", err);
        return 0;
    }

    printk("Bluetooth initialized successfully.\n");

    /*
     * Start advertising through the work queue.
     */

    k_work_schedule(
        &advertising_start_work,
        K_NO_WAIT
    );

    /*
     * Start sensor data generation.
     */

    k_work_schedule(
        &sensor_work,
        K_MSEC(100)
    );

    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
