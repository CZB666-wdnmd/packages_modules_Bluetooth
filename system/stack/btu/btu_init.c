/******************************************************************************
 *
 *  Copyright (C) 2000-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This module contains the routines that load and shutdown the core stack
 *  components.
 *
 ******************************************************************************/

#include "bt_target.h"
#include <string.h>
#include "dyn_mem.h"

#include "fixed_queue.h"
#include "btu.h"
#include "btm_int.h"
#include "sdpint.h"
#include "l2c_int.h"

#if (BLE_INCLUDED == TRUE)
#include "gatt_api.h"
#include "gatt_int.h"
#if SMP_INCLUDED == TRUE
#include "smp_int.h"
#endif
#endif


extern fixed_queue_t *btu_hci_msg_queue;
extern fixed_queue_t *btu_bta_msg_queue;

extern void PLATFORM_DisableHciTransport(UINT8 bDisable);
/*****************************************************************************
**                          V A R I A B L E S                                *
******************************************************************************/
const BD_ADDR   BT_BD_ANY = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*****************************************************************************
**                          F U N C T I O N S                                *
******************************************************************************/
/*****************************************************************************
**
** Function         btu_init_core
**
** Description      Initialize control block memory for each core component.
**
**
** Returns          void
**
******************************************************************************/
void btu_init_core(void)
{
    /* Initialize the mandatory core stack components */
    btm_init();

    l2c_init();

    sdp_init();

#if BLE_INCLUDED == TRUE
    gatt_init();
#if (defined(SMP_INCLUDED) && SMP_INCLUDED == TRUE)
    SMP_Init();
#endif
    btm_ble_init();
#endif
}

/*****************************************************************************
**
** Function         btu_free_core
**
** Description      Releases control block memory for each core component.
**
**
** Returns          void
**
******************************************************************************/
void btu_free_core(void)
{
      /* Free the mandatory core stack components */
#if BLE_INCLUDED == TRUE
      gatt_free();
#endif
}

/*****************************************************************************
**
** Function         BTE_StartUp
**
** Description      Initializes the BTU control block.
**
**                  NOTE: Must be called before creating any tasks
**                      (RPC, BTU, HCIT, APPL, etc.)
**
** Returns          void
**
******************************************************************************/
void BTE_StartUp(void)
{
    memset (&btu_cb, 0, sizeof (tBTU_CB));
    btu_cb.hcit_acl_pkt_size = BTU_DEFAULT_DATA_SIZE + HCI_DATA_PREAMBLE_SIZE;
#if (BLE_INCLUDED == TRUE)
    btu_cb.hcit_ble_acl_pkt_size = BTU_DEFAULT_BLE_DATA_SIZE + HCI_DATA_PREAMBLE_SIZE;
#endif
    btu_cb.trace_level = HCI_INITIAL_TRACE_LEVEL;

    for (int i = 0; i < BTU_MAX_LOCAL_CTRLS; ++i) {
      GKI_init_q(&btu_cb.hci_cmd_cb[i].cmd_xmit_q);
      GKI_init_q(&btu_cb.hci_cmd_cb[i].cmd_cmpl_q);
      btu_cb.hci_cmd_cb[i].cmd_window = 1;
    }
}


void BTE_ShutDown(void) {
  for (int i = 0; i < BTU_MAX_LOCAL_CTRLS; ++i) {
    while (!GKI_queue_is_empty(&btu_cb.hci_cmd_cb[i].cmd_xmit_q))
      GKI_freebuf(GKI_dequeue(&btu_cb.hci_cmd_cb[i].cmd_xmit_q));
    while (!GKI_queue_is_empty(&btu_cb.hci_cmd_cb[i].cmd_cmpl_q))
      GKI_freebuf(GKI_dequeue(&btu_cb.hci_cmd_cb[i].cmd_cmpl_q));
  }
  fixed_queue_free(btu_bta_msg_queue, NULL);
  fixed_queue_free(btu_hci_msg_queue, NULL);
}


/*****************************************************************************
**
** Function         BTU_AclPktSize
**
** Description      export the ACL packet size.
**
** Returns          UINT16
**
******************************************************************************/
UINT16 BTU_AclPktSize(void)
{
    return btu_cb.hcit_acl_pkt_size;
}
/*****************************************************************************
**
** Function         BTU_BleAclPktSize
**
** Description      export the BLE ACL packet size.
**
** Returns          UINT16
**
******************************************************************************/
UINT16 BTU_BleAclPktSize(void)
{
#if BLE_INCLUDED == TRUE
    return btu_cb.hcit_ble_acl_pkt_size;
#else
    return 0;
#endif
}

/*******************************************************************************
**
** Function         btu_uipc_rx_cback
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
void btu_uipc_rx_cback(BT_HDR *p_msg)
{
    BT_TRACE(TRACE_LAYER_BTM, TRACE_TYPE_DEBUG, "btu_uipc_rx_cback event 0x%x, len %d, offset %d",
		p_msg->event, p_msg->len, p_msg->offset);
    fixed_queue_enqueue(btu_hci_msg_queue, p_msg);
    // Signal the target thread work is ready.
    GKI_send_event(BTU_TASK, (UINT16)EVENT_MASK(BTU_HCI_RCV_MBOX));
}
