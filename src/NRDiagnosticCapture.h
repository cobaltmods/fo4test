#pragma once

// Developer-only: xmake f --nr_capture=y, then xmake build Upscaling.
// Default builds exclude the implementation, call sites and menu controls.

#include <d3d12.h>
#include <string>

namespace NRDiagnosticCapture
{
	void Arm();
	void MenuChanged(bool open);
	std::string Status();
	bool Requested();
	void Annotate(ID3D12GraphicsCommandList* list, std::string metadata);
	// All sources must be in COMMON; copies restore that state.
	bool Begin(ID3D12GraphicsCommandList* list, uint32_t frame, std::string metadata);
	void Copy(ID3D12GraphicsCommandList* list, const char* name, ID3D12Resource* source, UINT width, UINT height);
	void Finish(ID3D12GraphicsCommandList* list, bool nrSucceeded, bool srSucceeded, UINT passes);
	// Called only after the containing command list has been submitted and signalled.
	void Submitted(ID3D12GraphicsCommandList* list, ID3D12Fence* fence, UINT64 value);
}
