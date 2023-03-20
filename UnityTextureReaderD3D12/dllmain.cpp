// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdint.h>
#include "IUnityGraphics.h"
#include "IUnityInterface.h"
#include "IUnityGraphicsD3D12.h"
#include <iostream>

// Global pointers to Unity interfaces.
static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphicsD3D12v7* s_D3D12 = nullptr;


// Loads the plugin and initializes the interfaces.
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces * unityInterfaces)
{
    s_UnityInterfaces = unityInterfaces;
    s_D3D12 = s_UnityInterfaces->Get<IUnityGraphicsD3D12v7>();
}

// Unloads the plugin and resets the interface pointers.
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginUnload()
{
    s_D3D12 = nullptr;
    s_UnityInterfaces = nullptr;
}

// Retrieves pixel data from the texture.
extern "C" UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API GetPixelDataFromTexture(void* texturePtr)
{
    if (s_D3D12 == nullptr)
        return nullptr;

    // Get device and command queue from the D3D12 interface.
    ID3D12Device* device = s_D3D12->GetDevice();
    ID3D12CommandQueue* commandQueue = s_D3D12->GetCommandQueue();

    // Cast texture pointer and get its description.
    ID3D12Resource* texture = static_cast<ID3D12Resource*>(texturePtr);
    D3D12_RESOURCE_DESC desc = texture->GetDesc();

    // Update resource description for the readback buffer.
    D3D12_HEAP_PROPERTIES readbackHeapProperties = {};
    readbackHeapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    readbackHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    readbackHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC readbackDesc = {};
    readbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    readbackDesc.Alignment = 0;
    readbackDesc.Width = static_cast<UINT64>(desc.Width) * desc.Height * 4;
    readbackDesc.Height = 1;
    readbackDesc.DepthOrArraySize = 1;
    readbackDesc.MipLevels = 1;
    readbackDesc.Format = DXGI_FORMAT_UNKNOWN;
    readbackDesc.SampleDesc.Count = 1;
    readbackDesc.SampleDesc.Quality = 0;
    readbackDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    readbackDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Create a readback buffer.
    ID3D12Resource* readbackBuffer = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &readbackHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&readbackBuffer));
    if (FAILED(hr))
    {
        return nullptr;
    }

    // Create a command allocator and a command list.
    ID3D12CommandAllocator* commandAllocator = nullptr;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    if (FAILED(hr))
    {
        readbackBuffer->Release();
        return nullptr;
    }

    ID3D12GraphicsCommandList* commandList = nullptr;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
    if (FAILED(hr))
    {
        commandAllocator->Release();
        readbackBuffer->Release();
        return nullptr;
    }

    // Copy the texture to the readback buffer.
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = texture;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = readbackBuffer;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint.Offset = 0;
    dstLocation.PlacedFootprint.Footprint.Format = desc.Format;
    dstLocation.PlacedFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
    dstLocation.PlacedFootprint.Footprint.Height = static_cast<UINT>(desc.Height);
    dstLocation.PlacedFootprint.Footprint.Depth = 1;
    dstLocation.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(desc.Width) * 4;

    commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Close and execute the command list.
    commandList->Close();
    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, commandLists);

    // Create a fence and wait for the copy operation to complete.
    ID3D12Fence* fence = nullptr;
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr))
    {
        commandList->Release();
        commandAllocator->Release();
        readbackBuffer->Release();
        return nullptr;
    }

    UINT64 fenceValue = 1;
    commandQueue->Signal(fence, fenceValue);

    if (fence->GetCompletedValue() < fenceValue)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (eventHandle == nullptr)
        {
            fence->Release();
            commandList->Release();
            commandAllocator->Release();
            readbackBuffer->Release();
            return nullptr;
        }

        fence->SetEventOnCompletion(fenceValue, eventHandle);
        WaitForSingleObjectEx(eventHandle, INFINITE, FALSE);
        CloseHandle(eventHandle);
    }

    // Map the readback buffer to access its data.
    void* mappedData = nullptr;
    hr = readbackBuffer->Map(0, nullptr, &mappedData);
    if (FAILED(hr))
    {
        fence->Release();
        commandList->Release();
        commandAllocator->Release();
        readbackBuffer->Release();
        return nullptr;
    }

    // Copy the pixel data from the mapped resource.
    uint8_t* pixelData = new uint8_t[dstLocation.PlacedFootprint.Footprint.RowPitch * desc.Height];
    memcpy(pixelData, mappedData, dstLocation.PlacedFootprint.Footprint.RowPitch * desc.Height);

    // Unmap and release resources.
    readbackBuffer->Unmap(0, nullptr);
    fence->Release();
    commandList->Release();
    commandAllocator->Release();
    readbackBuffer->Release();

    return pixelData;
}

// Frees the memory allocated for pixel data.
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API FreePixelData(void* pixelData)
{
    if (pixelData != nullptr)
    {
        delete[] static_cast<uint8_t*>(pixelData);
    }
}
