/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_client.h"
#include "pragma/rendering/lighting/c_light_data_buffer_manager.hpp"

extern DLLCENGINE CEngine *c_engine;

using namespace pragma;

void BaseLightBufferManager::Initialize()
{
	if(m_bInitialized)
		return;
	m_bInitialized = true;
	DoInitialize();
}
void BaseLightBufferManager::Reset()
{
	m_masterBuffer = nullptr;
	m_bufferIndexToLightSource = {};
	m_bInitialized = false;

	m_maxCount = 0u;
}
CLightComponent *BaseLightBufferManager::GetLightByBufferIndex(uint32_t idx)
{
	return (idx < m_bufferIndexToLightSource.size()) ? m_bufferIndexToLightSource.at(idx) : nullptr;
}
std::size_t BaseLightBufferManager::GetMaxCount() const {return m_maxCount;}
const prosper::UniformResizableBuffer &BaseLightBufferManager::GetGlobalRenderBuffer() {return *m_masterBuffer;}

////////////////////

ShadowDataBufferManager &ShadowDataBufferManager::GetInstance()
{
	static ShadowDataBufferManager s_instance {};
	return s_instance;
}
void ShadowDataBufferManager::DoInitialize()
{
	auto &limits = c_engine->GetDevice().get_physical_device_properties().core_vk1_0_properties_ptr->limits;

	auto shadowDataSize = sizeof(ShadowBufferData);
	auto numShadows = static_cast<uint32_t>(umath::min(static_cast<uint64_t>(limits.max_storage_buffer_range /shadowDataSize),static_cast<uint64_t>(GameLimits::MaxAbsoluteShadowLights)));
	m_maxCount = numShadows;

	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
	createInfo.size = m_maxCount *shadowDataSize;

	m_masterBuffer = prosper::util::create_uniform_resizable_buffer(*c_engine,createInfo,shadowDataSize,createInfo.size,0.05f);
	m_masterBuffer->SetDebugName("light_shadow_data_buf");

	m_bufferIndexToLightSource.resize(m_maxCount,nullptr);
}
std::shared_ptr<prosper::Buffer> ShadowDataBufferManager::Request(CLightComponent &lightSource,const ShadowBufferData &bufferData)
{
	auto buf = m_masterBuffer->AllocateBuffer(&bufferData);
	if(buf != nullptr)
		m_bufferIndexToLightSource.at(buf->GetBaseIndex()) = &lightSource;
	return buf;
}
void ShadowDataBufferManager::Free(const std::shared_ptr<prosper::Buffer> &renderBuffer)
{
	m_bufferIndexToLightSource.at(renderBuffer->GetBaseIndex()) = nullptr;
}

////////////////////

LightDataBufferManager &LightDataBufferManager::GetInstance()
{
	static LightDataBufferManager s_instance {};
	return s_instance;
}
void LightDataBufferManager::DoInitialize()
{
	auto &limits = c_engine->GetDevice().get_physical_device_properties().core_vk1_0_properties_ptr->limits;

	auto lightDataSize = sizeof(LightBufferData);
	auto numLights = static_cast<uint32_t>(umath::min(static_cast<uint64_t>(limits.max_storage_buffer_range /lightDataSize),static_cast<uint64_t>(GameLimits::MaxAbsoluteLights)));
	m_maxCount = numLights;

	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
	createInfo.size = m_maxCount *lightDataSize;
	m_masterBuffer = prosper::util::create_uniform_resizable_buffer(*c_engine,createInfo,lightDataSize,createInfo.size,0.05f);
	m_masterBuffer->SetDebugName("light_data_buf");

	m_bufferIndexToLightSource.resize(m_maxCount,nullptr);
}
void LightDataBufferManager::Reset()
{
	m_lightDataBuffers = {};
	m_highestBufferIndexInUse = std::numeric_limits<uint32_t>::max();
}
std::shared_ptr<prosper::Buffer> LightDataBufferManager::Request(CLightComponent &lightSource,const LightBufferData &bufferData)
{
	std::shared_ptr<prosper::Buffer> renderBuffer = nullptr;
	if((m_highestBufferIndexInUse +1) < m_lightDataBuffers.size() && m_lightDataBuffers.at(m_highestBufferIndexInUse +1) != nullptr)
	{
		renderBuffer = m_lightDataBuffers.at(m_highestBufferIndexInUse +1);
		renderBuffer->Write(0ull,sizeof(bufferData),&bufferData);
	}
	else
		renderBuffer = m_masterBuffer->AllocateBuffer(&bufferData);

	auto baseIndex = renderBuffer->GetBaseIndex();
	m_bufferIndexToLightSource.at(baseIndex) = &lightSource;
	assert(baseIndex >= (m_highestBufferIndexInUse +1));
	if(baseIndex < (m_highestBufferIndexInUse +1))
		throw std::logic_error("Light source buffer index " +std::to_string(baseIndex) +" exceeds highest lights buffer index in use (" +std::to_string(m_highestBufferIndexInUse) +")!");
	m_highestBufferIndexInUse = baseIndex;

	m_lightDataBuffers.resize(m_masterBuffer->GetTotalInstanceCount(),nullptr);
	m_lightDataBuffers.at(baseIndex) = renderBuffer;
	return renderBuffer;
}
void LightDataBufferManager::Free(const std::shared_ptr<prosper::Buffer> &renderBuffer)
{
	auto baseIndex = renderBuffer->GetBaseIndex();
	m_bufferIndexToLightSource.at(baseIndex) = nullptr;

	if(baseIndex < m_highestBufferIndexInUse)
	{
		// Swap light source with highest buffer index with this light source
		// to make sure there are no unassigned slots in the lights buffer.
		auto *pLight = m_bufferIndexToLightSource.at(m_highestBufferIndexInUse);
		assert(pLight != nullptr);
		if(pLight == nullptr)
			throw std::logic_error("Expected valid light source at light buffer index " +std::to_string(m_highestBufferIndexInUse) +", but none available!");
		pLight->SetRenderBuffer(renderBuffer);
		// TODO: We can just copy the buffer data on the GPU instead
		c_engine->ScheduleRecordUpdateBuffer(renderBuffer,0ull,pLight->GetBufferData());

		m_bufferIndexToLightSource.at(m_highestBufferIndexInUse--) = nullptr;
		m_bufferIndexToLightSource.at(baseIndex) = pLight;
	}
	else if(baseIndex == m_highestBufferIndexInUse)
	{
		m_bufferIndexToLightSource.at(m_highestBufferIndexInUse) = nullptr;
		--m_highestBufferIndexInUse;
	}
	else
	{
		assert(m_highestBufferIndexInUse == 1);
		if(m_highestBufferIndexInUse != 1)
			throw std::logic_error("Light source buffer index " +std::to_string(baseIndex) +" exceeds highest lights buffer index in use (" +std::to_string(m_highestBufferIndexInUse) +")!");
		m_highestBufferIndexInUse = 0;
		//const auto flags = BufferFlags::None;
		//c_engine->ScheduleRecordUpdateBuffer(renderBuffer,offsetof(BufferData,flags),flags);
	}
}