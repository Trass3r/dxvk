#include "dxvk_device.h"

namespace dxvk {
  
  DxvkUnboundResources::DxvkUnboundResources(DxvkDevice* dev)
  : m_sampler       (createSampler(dev)),
    m_buffer        (createBuffer(dev)),
    m_bufferView    (createBufferView(dev, m_buffer)),
    m_image1D       (createImage(dev, VK_IMAGE_TYPE_1D, 1)),
    m_image2D       (createImage(dev, VK_IMAGE_TYPE_2D, 6)),
    m_image3D       (createImage(dev, VK_IMAGE_TYPE_3D, 1)),
    m_viewsSampled  (createImageViews(dev, VK_FORMAT_R32_SFLOAT)),
    m_viewsStorage  (createImageViews(dev, VK_FORMAT_R32_UINT)) {
    
  }
  
  
  DxvkUnboundResources::~DxvkUnboundResources() {
    
  }
  
  
  void DxvkUnboundResources::clearResources(DxvkDevice* dev) {
    const Rc<DxvkContext> ctx = dev->createContext();
    ctx->beginRecording(dev->createCommandList());
    
    this->clearBuffer(ctx, m_buffer);
    this->clearImage(ctx, m_image1D);
    this->clearImage(ctx, m_image2D);
    this->clearImage(ctx, m_image3D);
    
    dev->submitCommandList(
      ctx->endRecording(nullptr),
      nullptr, 0, nullptr);
  }
  
  
  Rc<DxvkSampler> DxvkUnboundResources::createSampler(DxvkDevice* dev) {
    DxvkSamplerKey samplerKey = {};
    samplerKey.setFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    samplerKey.setAddressModes(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    samplerKey.setLodRange(-256.0f, 256.0f, 0);
    samplerKey.setAniso(0);
    samplerKey.setDepthCompare(VK_FALSE, VK_COMPARE_OP_NEVER);
    samplerKey.setReduction(VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE);
    samplerKey.setBorderColor(VkClearColorValue());
    samplerKey.setUsePixelCoordinates(VK_FALSE);
    return dev->createSampler(samplerKey);
  }
  
  
  Rc<DxvkBuffer> DxvkUnboundResources::createBuffer(DxvkDevice* dev) {
    DxvkBufferCreateInfo info;
    info.size       = MaxUniformBufferSize;
    info.usage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                    | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                    | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                    | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                    | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
                    | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT
                    | VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    info.stages     = VK_PIPELINE_STAGE_TRANSFER_BIT
                    | dev->getShaderPipelineStages();
    info.access     = VK_ACCESS_UNIFORM_READ_BIT
                    | VK_ACCESS_SHADER_READ_BIT
                    | VK_ACCESS_SHADER_WRITE_BIT;
    info.debugName  = "Null buffer";
    
    Rc<DxvkBuffer> buffer = dev->createBuffer(info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    std::memset(buffer->mapPtr(0), 0, info.size);
    return buffer;
  }
  
  
  Rc<DxvkBufferView> DxvkUnboundResources::createBufferView(
          DxvkDevice*     dev,
    const Rc<DxvkBuffer>& buffer) {
    DxvkBufferViewKey info;
    info.format = VK_FORMAT_R32_UINT;
    info.offset = 0;
    info.size   = buffer->info().size;
    info.usage  = buffer->info().usage;
    return buffer->createView(info);
  }
  
  
  Rc<DxvkImage> DxvkUnboundResources::createImage(
          DxvkDevice*     dev,
          VkImageType     type,
          uint32_t        layers) {
    DxvkImageCreateInfo info;
    info.type        = type;
    info.format      = VK_FORMAT_R32_UINT;
    info.flags       = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    info.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    info.extent      = { 1, 1, 1 };
    info.numLayers   = layers;
    info.mipLevels   = 1;
    info.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                     | VK_IMAGE_USAGE_SAMPLED_BIT
                     | VK_IMAGE_USAGE_STORAGE_BIT;
    info.stages      = VK_PIPELINE_STAGE_TRANSFER_BIT
                     | dev->getShaderPipelineStages();
    info.access      = VK_ACCESS_SHADER_READ_BIT;
    info.layout      = VK_IMAGE_LAYOUT_GENERAL;
    info.tiling      = VK_IMAGE_TILING_OPTIMAL;
    info.debugName   = "Null Image";
    
    if (type == VK_IMAGE_TYPE_2D)
      info.flags       |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    return dev->createImage(info,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }
  
  
  Rc<DxvkImageView> DxvkUnboundResources::createImageView(
          DxvkDevice*     dev,
    const Rc<DxvkImage>&  image,
          VkFormat        format,
          VkImageViewType type,
          uint32_t        layers) {
    DxvkImageViewKey info;
    info.viewType = type;
    info.format = format;
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    info.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
    info.mipIndex = 0;
    info.mipCount = 1;
    info.layerIndex = 0;
    info.layerCount = layers;

    return image->createView(info);
  }
  
  
  DxvkUnboundResources::UnboundViews DxvkUnboundResources::createImageViews(DxvkDevice* dev, VkFormat format) {
    UnboundViews result;
    result.view1D      = createImageView(dev, m_image1D, format, VK_IMAGE_VIEW_TYPE_1D,         1);
    result.view1DArr   = createImageView(dev, m_image1D, format, VK_IMAGE_VIEW_TYPE_1D_ARRAY,   1);
    result.view2D      = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_2D,         1);
    result.view2DArr   = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_2D_ARRAY,   1);
    result.viewCube    = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_CUBE,       6);
    result.viewCubeArr = createImageView(dev, m_image2D, format, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, 6);
    result.view3D      = createImageView(dev, m_image3D, format, VK_IMAGE_VIEW_TYPE_3D,         1);
    return result;
  }


  DxvkImageView* DxvkUnboundResources::getImageView(VkImageViewType type, bool sampled) const {
    auto views = sampled ? &m_viewsSampled : &m_viewsStorage;

    switch (type) {
      case VK_IMAGE_VIEW_TYPE_1D:         return views->view1D.ptr();
      case VK_IMAGE_VIEW_TYPE_1D_ARRAY:   return views->view1DArr.ptr();
      // When implicit samplers are unbound -- we assume 2D in the shader.
      case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
      case VK_IMAGE_VIEW_TYPE_2D:         return views->view2D.ptr();
      case VK_IMAGE_VIEW_TYPE_2D_ARRAY:   return views->view2DArr.ptr();
      case VK_IMAGE_VIEW_TYPE_CUBE:       return views->viewCube.ptr();
      case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: return views->viewCubeArr.ptr();
      case VK_IMAGE_VIEW_TYPE_3D:         return views->view3D.ptr();
      default:                            return nullptr;
    }
  }
  
  
  void DxvkUnboundResources::clearBuffer(
    const Rc<DxvkContext>&  ctx,
    const Rc<DxvkBuffer>&   buffer) {
    ctx->initBuffer(buffer);
  }
  
  
  void DxvkUnboundResources::clearImage(
    const Rc<DxvkContext>&  ctx,
    const Rc<DxvkImage>&    image) {
    ctx->initImage(image,
      VkImageSubresourceRange {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, image->info().mipLevels,
        0, image->info().numLayers },
      VK_IMAGE_LAYOUT_UNDEFINED);
  }
  
}