/*
 * MVKDescriptorSet.h
 *
 * Copyright (c) 2015-2024 The Brenwill Workshop Ltd. (http://www.brenwill.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "MVKDescriptor.h"
#include "MVKSmallVector.h"
#include "MVKBitArray.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>

class MVKDescriptorPool;
class MVKPipelineLayout;
class MVKCommandEncoder;
class MVKResourcesCommandEncoderState;


#pragma mark -
#pragma mark MVKMetalArgumentBuffer

/**
 * Helper object to handle the placement of resources into a Metal Argument Buffer
 * in a consistent manner, whether or not a MTLArgumentEncoder is required.
 */
typedef struct MVKMetalArgumentBuffer {
	void setBuffer(id<MTLBuffer> mtlBuff, NSUInteger offset, uint32_t index);
	void setTexture(id<MTLTexture> mtlTex, uint32_t index);
	void setSamplerState(id<MTLSamplerState> mtlSamp, uint32_t index);
	id<MTLBuffer> getMetalArgumentBuffer() { return _mtlArgumentBuffer; }
	NSUInteger getMetalArgumentBufferOffset() { return _mtlArgumentBufferOffset; }
	void setArgumentBuffer(id<MTLBuffer> mtlArgBuff, NSUInteger mtlArgBuffOfst, id<MTLArgumentEncoder> mtlArgEnc);
	~MVKMetalArgumentBuffer();
protected:
	void* getArgumentPointer(uint32_t index) const;
	id<MTLArgumentEncoder> _mtlArgumentEncoder = nil;
	id<MTLBuffer> _mtlArgumentBuffer = nil;
	NSUInteger _mtlArgumentBufferOffset = 0;
} MVKMetalArgumentBuffer;


#pragma mark -
#pragma mark MVKDescriptorSetLayout

/** Represents a Vulkan descriptor set layout. */
class MVKDescriptorSetLayout : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT; }

	/** Encodes this descriptor set layout and the specified descriptor set on the specified command encoder. */
	void bindDescriptorSet(MVKCommandEncoder* cmdEncoder,
						   VkPipelineBindPoint pipelineBindPoint,
						   uint32_t descSetIndex,
						   MVKDescriptorSet* descSet,
						   MVKShaderResourceBinding& dslMTLRezIdxOffsets,
						   MVKArrayRef<uint32_t> dynamicOffsets,
						   uint32_t& dynamicOffsetIndex);

	/** Encodes this descriptor set layout and the specified descriptor updates on the specified command encoder immediately. */
	void pushDescriptorSet(MVKCommandEncoder* cmdEncoder,
						   VkPipelineBindPoint pipelineBindPoint,
						   MVKArrayRef<VkWriteDescriptorSet> descriptorWrites,
						   MVKShaderResourceBinding& dslMTLRezIdxOffsets);


	/** Encodes this descriptor set layout and the updates from the given template on the specified command encoder immediately. */
	void pushDescriptorSet(MVKCommandEncoder* cmdEncoder,
						   MVKDescriptorUpdateTemplate* descUpdateTemplates,
						   const void* pData,
						   MVKShaderResourceBinding& dslMTLRezIdxOffsets);


	/** Populates the specified shader conversion config, at the specified DSL index. */
	void populateShaderConversionConfig(mvk::SPIRVToMSLConversionConfiguration& shaderConfig,
                                        MVKShaderResourceBinding& dslMTLRezIdxOffsets,
                                        uint32_t descSetIndex);

	/**
	 * Populates the bindings in this descriptor set layout used by the shader.
	 * Returns false if the shader does not use the descriptor set at all.
	 */
	bool populateBindingUse(MVKBitArray& bindingUse,
							mvk::SPIRVToMSLConversionConfiguration& context,
							MVKShaderStage stage,
							uint32_t descSetIndex);

	/** Returns the number of bindings. */
	uint32_t getBindingCount() { return (uint32_t)_bindings.size(); }

	/** Returns the binding at the index in a descriptor set layout. */
	MVKDescriptorSetLayoutBinding* getBindingAt(uint32_t index) { return &_bindings[index]; }

	/** Overridden becasue descriptor sets may be marked as discrete and not use an argument buffer. */
	bool isUsingMetalArgumentBuffers() override;

	MVKDescriptorSetLayout(MVKDevice* device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo);

	~MVKDescriptorSetLayout();

protected:

	friend class MVKDescriptorSetLayoutBinding;
	friend class MVKPipelineLayout;
	friend class MVKDescriptorSet;
	friend class MVKDescriptorPool;

	void propagateDebugName() override {}
	uint32_t getDescriptorCount() { return _descriptorCount; }
	uint32_t getDescriptorIndex(uint32_t binding, uint32_t elementIndex = 0) { return getBinding(binding)->getDescriptorIndex(elementIndex); }
	MVKDescriptorSetLayoutBinding* getBinding(uint32_t binding) { return &_bindings[_bindingToIndex[binding]]; }
	const VkDescriptorBindingFlags* getBindingFlags(const VkDescriptorSetLayoutCreateInfo* pCreateInfo);
	uint32_t getBufferSizeBufferArgBuferIndex() { return _mtlResourceCount; }
	std::string getLogDescription();

	MVKSmallVector<MVKDescriptorSetLayoutBinding> _bindings;
	std::unordered_map<uint32_t, uint32_t> _bindingToIndex;
	MVKShaderResourceBinding _mtlResourceCounts;
	NSArray<MTLArgumentDescriptor*>* _mtlArgumentEncoderArgs = nil;
	uint64_t _mtlArgumentBufferEncodedSize = 0;
	uint32_t _descriptorCount = 0;
	uint32_t _mtlResourceCount = 0;
	int32_t _maxBufferIndex = -1;
	bool _isPushDescriptorLayout = false;
};


#pragma mark -
#pragma mark MVKDescriptorSet

/** Represents a Vulkan descriptor set. */
class MVKDescriptorSet : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_DESCRIPTOR_SET; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT; }

	/** Returns the layout that defines this descriptor set. */
	MVKDescriptorSetLayout* getLayout() { return _layout; }

	/** Returns the descriptor type for the specified binding number. */
	VkDescriptorType getDescriptorType(uint32_t binding);

	/** Updates the resource bindings in this instance from the specified content. */
	template<typename DescriptorAction>
	void write(const DescriptorAction* pDescriptorAction, size_t srcStride, const void* pData);

	/** 
	 * Reads the resource bindings defined in the specified content 
	 * from this instance into the specified collection of bindings.
	 */
	void read(const VkCopyDescriptorSet* pDescriptorCopies,
			  VkDescriptorImageInfo* pImageInfo,
			  VkDescriptorBufferInfo* pBufferInfo,
			  VkBufferView* pTexelBufferView,
			  VkWriteDescriptorSetInlineUniformBlockEXT* pInlineUniformBlock);

	/** Returns the descriptor at an index. */
	MVKDescriptor* getDescriptorAt(uint32_t descIndex) { return _descriptors[descIndex]; }

	/** Returns the number of descriptors in this descriptor set. */
	uint32_t getDescriptorCount() { return (uint32_t)_descriptors.size(); }

	/** Returns the number of descriptors in this descriptor set that use dynamic offsets. */
	uint32_t getDynamicOffsetDescriptorCount() { return _dynamicOffsetDescriptorCount; }

	/** Returns true if this descriptor set is using a Metal argument buffer. */
	bool hasMetalArgumentBuffer() { return _layout->isUsingMetalArgumentBuffers(); };

	/** Returns the argument buffer helper object used by this descriptor set. */
	MVKMetalArgumentBuffer& getMetalArgumentBuffer() { return _argumentBuffer; }

	/** Encode the buffer sizes auxiliary buffer to the GPU. */
	void encodeAuxBufferUsage(MVKResourcesCommandEncoderState* rezEncState, MVKShaderStage stage);

	MVKDescriptorSet(MVKDescriptorPool* pool);

protected:
	friend class MVKDescriptorSetLayoutBinding;
	friend class MVKDescriptorPool;
	friend class MVKBufferDescriptor;
	friend class MVKInlineUniformBlockDescriptor;

	void propagateDebugName() override {}
	MVKDescriptor* getDescriptor(uint32_t binding, uint32_t elementIndex = 0);
	VkResult allocate(MVKDescriptorSetLayout* layout,
					  uint32_t variableDescriptorCount,
					  NSUInteger mtlArgBufferOffset);
	void free(bool isPoolReset);
	MVKMTLBufferAllocation* acquireMTLBufferRegion(NSUInteger length);
	void setBufferSize(uint32_t descIdx, uint32_t value);

	MVKDescriptorPool* _pool;
	MVKDescriptorSetLayout* _layout;
	MVKSmallVector<MVKDescriptor*> _descriptors;
	MVKMetalArgumentBuffer _argumentBuffer;
	MVKMTLBufferAllocation* _bufferSizesBuffer = nullptr;
	uint32_t _dynamicOffsetDescriptorCount;
	uint32_t _variableDescriptorCount;
};


#pragma mark -
#pragma mark MVKDescriptorTypePool

/** Support class for MVKDescriptorPool that holds a pool of instances of a single concrete descriptor class. */
template<class DescriptorClass>
class MVKDescriptorTypePool : public MVKBaseObject {

public:

	MVKVulkanAPIObject* getVulkanAPIObject() override { return nullptr; };

	MVKDescriptorTypePool(size_t poolSize);

protected:
	friend class MVKDescriptorPool;

	VkResult allocateDescriptor(MVKDescriptor** pMVKDesc, MVKDescriptorPool* pool);
	void freeDescriptor(MVKDescriptor* mvkDesc, MVKDescriptorPool* pool);
	void reset();

	MVKSmallVector<DescriptorClass> _descriptors;
	MVKBitArray _availability;
};


#pragma mark -
#pragma mark MVKDescriptorPool

/** Represents a Vulkan descriptor pool. */
class MVKDescriptorPool : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_DESCRIPTOR_POOL; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT; }

	/** Allocates descriptor sets. */
	VkResult allocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo,
									VkDescriptorSet* pDescriptorSets);

	/** Free's up the specified descriptor set. */
	VkResult freeDescriptorSets(uint32_t count, const VkDescriptorSet* pDescriptorSets);

	/** Destroys all currently allocated descriptor sets. */
	VkResult reset(VkDescriptorPoolResetFlags flags);

	MVKDescriptorPool(MVKDevice* device, const VkDescriptorPoolCreateInfo* pCreateInfo);

	~MVKDescriptorPool() override;

protected:
	friend class MVKDescriptorSet;
	template<class> friend class MVKDescriptorTypePool;

	void propagateDebugName() override {}
	const uint32_t* getVariableDecriptorCounts(const VkDescriptorSetAllocateInfo* pAllocateInfo);
	VkResult allocateDescriptorSet(MVKDescriptorSetLayout* mvkDSL, uint32_t variableDescriptorCount, VkDescriptorSet* pVKDS);
	void freeDescriptorSet(MVKDescriptorSet* mvkDS, bool isPoolReset);
	VkResult allocateDescriptor(VkDescriptorType descriptorType, MVKDescriptor** pMVKDesc);
	void freeDescriptor(MVKDescriptor* mvkDesc);
	void initMetalArgumentBuffer(const VkDescriptorPoolCreateInfo* pCreateInfo);
	NSUInteger getMetalArgumentBufferEncodedResourceStorageSize(NSUInteger bufferCount, NSUInteger textureCount, NSUInteger samplerCount);
	MTLArgumentDescriptor* getMTLArgumentDescriptor(MTLDataType resourceType, NSUInteger argIndex, NSUInteger count);
	size_t getPoolSize(const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorType descriptorType);

    bool _hasPooledDescriptors;
	MVKSmallVector<MVKDescriptorSet> _descriptorSets;
	MVKBitArray _descriptorSetAvailablility;
	id<MTLBuffer> _metalArgumentBuffer;
	NSUInteger _nextMetalArgumentBufferOffset;
	MVKMTLBufferAllocator _mtlBufferAllocator;

	MVKDescriptorTypePool<MVKUniformBufferDescriptor> _uniformBufferDescriptors;
	MVKDescriptorTypePool<MVKStorageBufferDescriptor> _storageBufferDescriptors;
	MVKDescriptorTypePool<MVKUniformBufferDynamicDescriptor> _uniformBufferDynamicDescriptors;
	MVKDescriptorTypePool<MVKStorageBufferDynamicDescriptor> _storageBufferDynamicDescriptors;
	MVKDescriptorTypePool<MVKInlineUniformBlockDescriptor> _inlineUniformBlockDescriptors;
	MVKDescriptorTypePool<MVKSampledImageDescriptor> _sampledImageDescriptors;
	MVKDescriptorTypePool<MVKStorageImageDescriptor> _storageImageDescriptors;
	MVKDescriptorTypePool<MVKInputAttachmentDescriptor> _inputAttachmentDescriptors;
	MVKDescriptorTypePool<MVKSamplerDescriptor> _samplerDescriptors;
	MVKDescriptorTypePool<MVKCombinedImageSamplerDescriptor> _combinedImageSamplerDescriptors;
	MVKDescriptorTypePool<MVKUniformTexelBufferDescriptor> _uniformTexelBufferDescriptors;
	MVKDescriptorTypePool<MVKStorageTexelBufferDescriptor> _storageTexelBufferDescriptors;
};


#pragma mark -
#pragma mark MVKDescriptorUpdateTemplate

/** Represents a Vulkan descriptor update template. */
class MVKDescriptorUpdateTemplate : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT; }

	/** Get the nth update template entry. */
	const VkDescriptorUpdateTemplateEntry* getEntry(uint32_t n) const;

	/** Get the total number of entries. */
	uint32_t getNumberOfEntries() const;

	/** Get the type of this template. */
	VkDescriptorUpdateTemplateType getType() const;

	/** Get the bind point of this template */
	VkPipelineBindPoint getBindPoint() const { return _pipelineBindPoint; }

	/** Constructs an instance for the specified device. */
	MVKDescriptorUpdateTemplate(MVKDevice* device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo);

	/** Destructor. */
	~MVKDescriptorUpdateTemplate() override = default;

protected:
	void propagateDebugName() override {}

	VkPipelineBindPoint _pipelineBindPoint;
	VkDescriptorUpdateTemplateType _type;
	MVKSmallVector<VkDescriptorUpdateTemplateEntry, 1> _entries;
};

#pragma mark -
#pragma mark Support functions

/** Updates the resource bindings in the descriptor sets inditified in the specified content. */
void mvkUpdateDescriptorSets(uint32_t writeCount,
							const VkWriteDescriptorSet* pDescriptorWrites,
							uint32_t copyCount,
							const VkCopyDescriptorSet* pDescriptorCopies);

/** Updates the resource bindings in the given descriptor set from the specified template. */
void mvkUpdateDescriptorSetWithTemplate(VkDescriptorSet descriptorSet,
										VkDescriptorUpdateTemplate updateTemplate,
										const void* pData);
