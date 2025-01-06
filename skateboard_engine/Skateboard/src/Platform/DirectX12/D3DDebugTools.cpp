#include "sktbdpch.h"
#include "D3DDebugTools.h"

#include <pix3.h>

UINT D3DDebugTools::s_CapturesTaken = 0;

const char* BreadCrumbOpToStr(D3D12_AUTO_BREADCRUMB_OP op)
{
	switch (op)
	{
	case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:										return "SETMARKER";
	case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:										return "BEGINEVENT";
	case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:											return "ENDEVENT";
	case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:									return "DRAWINSTANCED";
	case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:								return "DRAWINDEXEDINSTANCED";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:									return "EXECUTEINDIRECT";
	case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:											return "DISPATCH";
	case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:									return "COPYBUFFERREGION";
	case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:								return "COPYTEXTUREREGION";
	case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:										return "COPYRESOURCE";
	case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:										return "COPYTILES";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:								return "RESOLVESUBRESOURCE";
	case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:							return "CLEARRENDERTARGETVIEW";
	case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:							return "CLEARUNORDEREDACCESSVIEW";
	case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:							return "CLEARDEPTHSTENCILVIEW";
	case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:									return "RESOURCEBARRIER";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:									return "EXECUTEBUNDLE";
	case D3D12_AUTO_BREADCRUMB_OP_PRESENT:											return "PRESENT";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:									return "RESOLVEQUERYDATA";
	case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:									return "BEGINSUBMISSION";
	case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:									return "ENDSUBMISSION";
	case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:										return "DECODEFRAME";
	case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:									return "PROCESSFRAMES";
	case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:								return "ATOMICCOPYBUFFERUINT";
	case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:							return "ATOMICCOPYBUFFERUINT64";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:							return "RESOLVESUBRESOURCEREGION";
	case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:								return "WRITEBUFFERIMMEDIATE";
	case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:										return "DECODEFRAME1";
	case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:						return "SETPROTECTEDRESOURCESESSION";
	case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:										return "DECODEFRAME2";
	case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:									return "PROCESSFRAMES1";
	case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:				return "BUILDRAYTRACINGACCELERATIONSTRUCTURE";
	case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:	return "EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
	case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:				return "COPYRAYTRACINGACCELERATIONSTRUCTURE";
	case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:										return "DISPATCHRAYS";
	case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:							return "INITIALIZEMETACOMMAND";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:								return "EXECUTEMETACOMMAND";
	case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:									return "ESTIMATEMOTION";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:							return "RESOLVEMOTIONVECTORHEAP";
	case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:								return "SETPIPELINESTATE1";
	case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:						return "INITIALIZEEXTENSIONCOMMAND";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:							return "EXECUTEEXTENSIONCOMMAND";
	case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:										return "DISPATCHMESH";
	case D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME:										return "ENCODEFRAME";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA:						return "RESOLVEENCODEROUTPUTMETADATA";
	case D3D12_AUTO_BREADCRUMB_OP_BARRIER:											return "BARRIER";
	}
	return "UNKNOWN";
}

const char* AllocationTypeToStr(D3D12_DRED_ALLOCATION_TYPE allocation)
{
	switch (allocation)
	{
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE:								return "COMMAND_QUEUE";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR:							return "COMMAND_ALLOCATOR";
		case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:								return "PIPELINE_STATE";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:								return "COMMAND_LIST";
		case D3D12_DRED_ALLOCATION_TYPE_FENCE:										return "FENCE";
		case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:							return "DESCRIPTOR_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_HEAP:										return "HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP:									return "QUERY_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE:							return "COMMAND_SIGNATURE";
		case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY:							return "PIPELINE_LIBRARY";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER:								return "VIDEO_DECODER";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR:							return "VIDEO_PROCESSOR";
		case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:									return "RESOURCE";
		case D3D12_DRED_ALLOCATION_TYPE_PASS:										return "PASS";
		case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION:								return "CRYPTOSESSION";
		case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY:						return "CRYPTOSESSIONPOLICY";
		case D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION:					return "PROTECTEDRESOURCESESSION";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP:							return "VIDEO_DECODER_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL:								return "COMMAND_POOL";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER:							return "COMMAND_RECORDER";
		case D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT:								return "STATE_OBJECT";
		case D3D12_DRED_ALLOCATION_TYPE_METACOMMAND:								return "METACOMMAND";
		case D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP:							return "SCHEDULINGGROUP";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR:						return "VIDEO_MOTION_ESTIMATOR";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP:					return "VIDEO_MOTION_VECTOR_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND:					return "VIDEO_EXTENSION_COMMAND";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER:								return "VIDEO_ENCODER";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP:							return "VIDEO_ENCODER_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_INVALID:									return "INVALID";
	}
	return "UNKNOWN";
}



void D3DDebugTools::D3DMessageHandler(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
{
	switch (Severity)
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		SKTBD_CORE_CRITICAL(pDescription); break;
	case D3D12_MESSAGE_SEVERITY_ERROR:
		SKTBD_CORE_ERROR(pDescription); break;
	case D3D12_MESSAGE_SEVERITY_WARNING:
		SKTBD_CORE_WARN(pDescription); break;
	case D3D12_MESSAGE_SEVERITY_INFO:
		SKTBD_CORE_INFO(pDescription); break;
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
	default:
		SKTBD_CORE_TRACE(pDescription); break;
	}

	if (gD3DContext && gD3DContext->CheckDeviceRemovedStatus())
	{
		// Attempt to recover DRED
		ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
		D3D_CHECK_FAILURE(gD3DContext->Device()->QueryInterface(IID_PPV_ARGS(&pDred)));

		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
		D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput;
		D3D_CHECK_FAILURE(pDred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput));
		D3D_CHECK_FAILURE(pDred->GetPageFaultAllocationOutput(&DredPageFaultOutput));

		if (DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode)
		{
			LogAutoBreadcrumbs(DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode);
		}
		else
		{
			SKTBD_CORE_INFO("No auto-breadcrumbs data was retrieved from DRED.");
		}
		if (DredPageFaultOutput.PageFaultVA)
		{
			LogPageFault(&DredPageFaultOutput);
		}
		else
		{
			SKTBD_CORE_INFO("No page fault data was retrieved from DRED");
		}
	}

}


void D3DDebugTools::PIXGPUCaptureFrame(UINT frameCount)
{
	const std::wstring captureName = L"captures/capture" + std::to_wstring(s_CapturesTaken++) + L".wpix";
	const HRESULT result = PIXGpuCaptureNextFrames(captureName.c_str(), frameCount);
	if (SUCCEEDED(result))
	{
		SKTBD_CORE_INFO(L"Capture saved as: {}", captureName.c_str());
	}
	else
	{
		SKTBD_CORE_ERROR("Capture failed.");
	}
}



void D3DDebugTools::LogAutoBreadcrumbs(const D3D12_AUTO_BREADCRUMB_NODE1* breadcrumb)
{
	SKTBD_CORE_INFO("DRED Auto Breadcrumbs:");
	SKTBD_CORE_INFO("-------------");

	while (breadcrumb)
	{
		const UINT lastBreadcrumb = breadcrumb->pLastBreadcrumbValue ? *breadcrumb->pLastBreadcrumbValue : 0;
		if (breadcrumb->BreadcrumbCount && breadcrumb->BreadcrumbCount > lastBreadcrumb)
		{
			if (breadcrumb->pCommandQueueDebugNameW)
			{
				SKTBD_CORE_INFO(L"Command Queue: {}", breadcrumb->pCommandQueueDebugNameW);
			}
			if (breadcrumb->pCommandListDebugNameW)
			{
				SKTBD_CORE_INFO(L"Command List: {}", breadcrumb->pCommandListDebugNameW);
			}

			if (breadcrumb->pCommandHistory)
			{
				SKTBD_CORE_INFO("Operation History:");
				auto command = breadcrumb->pCommandHistory;
				for (UINT i = 0; i < breadcrumb->BreadcrumbCount && command; i++)
				{
					if (i == lastBreadcrumb)
					{
						SKTBD_CORE_WARN("--->{}", BreadCrumbOpToStr(*command));
					}
					else
					{
						SKTBD_CORE_INFO("    {}", BreadCrumbOpToStr(*command));
					}
					command++;
				}
			}
			SKTBD_CORE_INFO("-------------");
		}

		breadcrumb = breadcrumb->pNext;
	}
}


void D3DDebugTools::LogPageFault(D3D12_DRED_PAGE_FAULT_OUTPUT* pageFault)
{
	SKTBD_CORE_INFO("Page Fault VA: {}", pageFault->PageFaultVA);

	auto allocation = pageFault->pHeadExistingAllocationNode;
	if (allocation)
	{
		SKTBD_CORE_INFO("Existing objects with matching VA:");
		while (allocation)
		{
			SKTBD_CORE_INFO(L"	Name: {}", allocation->ObjectNameW);
			SKTBD_CORE_INFO("	Type: {}", AllocationTypeToStr(allocation->AllocationType));
			SKTBD_CORE_INFO("    -------------");
			allocation = allocation->pNext;
		}
	}
	else
	{
		SKTBD_CORE_INFO("No existing objects matching VA.");
	}

	allocation = pageFault->pHeadRecentFreedAllocationNode;
	if (allocation)
	{
		SKTBD_CORE_INFO("Recently freed objects with matching VA:");
		while (allocation)
		{
			SKTBD_CORE_INFO(L"	Name: {}", allocation->ObjectNameW);
			SKTBD_CORE_INFO("	Type: {}", AllocationTypeToStr(allocation->AllocationType));
			SKTBD_CORE_INFO("    -------------");
			allocation = allocation->pNext;
		}
	}
	else
	{
		SKTBD_CORE_INFO("No recently freed objects matching VA.");
	}
}
