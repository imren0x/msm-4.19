// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>
#include <linux/usb/gadget.h>

#include "core.h"
#include "gadget.h"

#if IS_ENABLED(CONFIG_USB_DWC3_GADGET) || IS_ENABLED(CONFIG_USB_DWC3_DUAL_ROLE)

int dwc3_msm_new_data_fifo_config(struct usb_ep *ep, unsigned long addr,
				  u32 size, u8 dst_pipe_idx);
int dwc3_msm_legacy_data_fifo_config(struct usb_ep *ep, unsigned long addr,
					 u32 size, u8 dst_pipe_idx);
int dwc3_msm_new_reset_dbm_ep(struct usb_ep *ep);
int dwc3_msm_legacy_reset_dbm_ep(struct usb_ep *ep);
int dwc3_msm_new_ep_config(struct usb_ep *ep, struct usb_request *request);
int dwc3_msm_legacy_ep_config(struct usb_ep *ep, struct usb_request *request);
int dwc3_msm_new_ep_unconfig(struct usb_ep *ep);
int dwc3_msm_legacy_ep_unconfig(struct usb_ep *ep);
bool dwc3_msm_new_reset_ep_after_lpm(struct usb_gadget *gadget);
bool dwc3_msm_legacy_reset_ep_after_lpm(struct usb_gadget *gadget);

int msm_data_fifo_config(struct usb_ep *ep, unsigned long addr, u32 size,
			 u8 dst_pipe_idx)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);

	if (dep->dwc->is_msm_legacy)
		return dwc3_msm_legacy_data_fifo_config(ep, addr, size,
						       dst_pipe_idx);

	return dwc3_msm_new_data_fifo_config(ep, addr, size, dst_pipe_idx);
}

int msm_dwc3_reset_dbm_ep(struct usb_ep *ep)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);

	if (dep->dwc->is_msm_legacy)
		return dwc3_msm_legacy_reset_dbm_ep(ep);

	return dwc3_msm_new_reset_dbm_ep(ep);
}
EXPORT_SYMBOL(msm_dwc3_reset_dbm_ep);

int msm_ep_config(struct usb_ep *ep, struct usb_request *request)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);

	if (dep->dwc->is_msm_legacy)
		return dwc3_msm_legacy_ep_config(ep, request);

	return dwc3_msm_new_ep_config(ep, request);
}
EXPORT_SYMBOL(msm_ep_config);

int msm_ep_unconfig(struct usb_ep *ep)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);

	if (dep->dwc->is_msm_legacy)
		return dwc3_msm_legacy_ep_unconfig(ep);

	return dwc3_msm_new_ep_unconfig(ep);
}
EXPORT_SYMBOL(msm_ep_unconfig);

bool msm_dwc3_reset_ep_after_lpm(struct usb_gadget *gadget)
{
	struct dwc3 *dwc = container_of(gadget, struct dwc3, gadget);

	if (dwc->is_msm_legacy)
		return dwc3_msm_legacy_reset_ep_after_lpm(gadget);

	return dwc3_msm_new_reset_ep_after_lpm(gadget);
}
EXPORT_SYMBOL(msm_dwc3_reset_ep_after_lpm);

#endif