/*
   metal_app.m
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] structs
// [SECTION] globals
// [SECTION] pl_app_setup
// [SECTION] pl_app_shutdown
// [SECTION] pl_app_resize
// [SECTION] pl_app_render
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include "metal_pl.h"
#include "metal_pl_graphics.h"
#include "metal_pl_drawing.h"
#include "pl_ds.h"

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct
{
    uint32_t                    viewportSize[2]; // required by apple_pl.m
    id<MTLTexture>              depthTarget;
    MTLRenderPassDescriptor*    drawableRenderDescriptor;
    plDrawContext*              ctx;
    plDrawList*                 drawlist;
    plDrawLayer*                fgDrawLayer;
    plDrawLayer*                bgDrawLayer;
    plFontAtlas                 fontAtlas;
} plAppData;

//-----------------------------------------------------------------------------
// [SECTION] globals
//-----------------------------------------------------------------------------

static plAppData gAppData = {0};

//-----------------------------------------------------------------------------
// [SECTION] pl_app_setup
//-----------------------------------------------------------------------------

void
pl_app_setup()
{
    // create command queue
    gGraphics.cmdQueue = [gDevice.device newCommandQueue];

    // render pass descriptor
    gAppData.drawableRenderDescriptor = [MTLRenderPassDescriptor new];

    // color attachment
    gAppData.drawableRenderDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    gAppData.drawableRenderDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    gAppData.drawableRenderDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.01, 0, 0, 1);

    // depth attachment
    gAppData.drawableRenderDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
    gAppData.drawableRenderDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
    gAppData.drawableRenderDescriptor.depthAttachment.clearDepth = 1.0;

    // create draw context
    gAppData.ctx = pl_create_draw_context_metal(gDevice.device);

    // create draw list & layers
    gAppData.drawlist = pl_create_drawlist(gAppData.ctx);
    gAppData.bgDrawLayer = pl_request_draw_layer(gAppData.drawlist, "Background Layer");
    gAppData.fgDrawLayer = pl_request_draw_layer(gAppData.drawlist, "Foreground Layer");

    // create font atlas
    pl_add_default_font(&gAppData.fontAtlas);

    plFontConfig fontConfig = {
        .sdf = true,
        .fontSize = 42.0f,
        .hOverSampling = 1,
        .vOverSampling = 1,
        .onEdgeValue = 255,
        .sdfPadding = 1
    };
    
    plFontRange range = {
        .firstCodePoint = 0x0020,
        .charCount = 0x00FF - 0x0020
    };
    pl_sb_push(fontConfig.sbRanges, range);
    pl_add_font_from_file_ttf(&gAppData.fontAtlas, fontConfig, "Cousine-Regular.ttf");
    pl_build_font_atlas(gAppData.ctx, &gAppData.fontAtlas);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

void
pl_app_shutdown()
{
    pl_cleanup_font_atlas(&gAppData.fontAtlas);
    pl_cleanup_draw_context(gAppData.ctx);   
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

void
pl_app_resize()
{    
    // recreate depth texture
    MTLTextureDescriptor *depthTargetDescriptor = [MTLTextureDescriptor new];
    depthTargetDescriptor.width       = gAppData.viewportSize[0];
    depthTargetDescriptor.height      = gAppData.viewportSize[1];
    depthTargetDescriptor.pixelFormat = MTLPixelFormatDepth32Float;
    depthTargetDescriptor.storageMode = MTLStorageModePrivate;
    depthTargetDescriptor.usage       = MTLTextureUsageRenderTarget;
    gAppData.depthTarget = [gDevice.device newTextureWithDescriptor:depthTargetDescriptor];
    gAppData.drawableRenderDescriptor.depthAttachment.texture = gAppData.depthTarget;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_render
//-----------------------------------------------------------------------------

void
pl_app_render()
{
    gGraphics.currentFrame++;

    // request command buffer
    id<MTLCommandBuffer> commandBuffer = [gGraphics.cmdQueue commandBuffer];

    // get next drawable
    id<CAMetalDrawable> currentDrawable = [gGraphics.metalLayer nextDrawable];

    if(!currentDrawable)
        return;

    // set colorattachment to next drawable
    gAppData.drawableRenderDescriptor.colorAttachments[0].texture = currentDrawable.texture;

    pl_new_draw_frame_metal(gAppData.ctx, gAppData.drawableRenderDescriptor);

    // create render command encoder
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:gAppData.drawableRenderDescriptor];

    // draw commands
    pl_add_text(gAppData.fgDrawLayer, &gAppData.fontAtlas.sbFonts[0], 13.0f, (plVec2){10.0f, 100.0f}, (plVec4){0.1f, 0.5f, 0.0f, 1.0f}, "Bitmap Font", 0.0f);
    pl_add_text(gAppData.fgDrawLayer, &gAppData.fontAtlas.sbFonts[1], 42.0f, (plVec2){10.0f, 10.0f}, (plVec4){0.1f, 0.5f, 0.0f, 1.0f}, "SDF Font", 0.0f);
    
    pl_add_triangle_filled(gAppData.bgDrawLayer, (plVec2){500.0f, 10.0f}, (plVec2){500.0f, 200.0f}, (plVec2){700.0f, 200.0f}, (plVec4){1.0f, 0.0f, 0.0f, 1.0f});
    pl_add_triangle_filled(gAppData.bgDrawLayer, (plVec2){500.0f, 10.0f}, (plVec2){700.0f, 200.0f}, (plVec2){700.0f, 10.0f}, (plVec4){0.0f, 1.0f, 0.0f, 1.0f});

    plVec2 textSize0 = pl_calculate_text_size(&gAppData.fontAtlas.sbFonts[0], 13.0f, "Bitmap Font", 0.0f);
    plVec2 textSize1 = pl_calculate_text_size(&gAppData.fontAtlas.sbFonts[1], 42.0f, "SDF Font", 0.0f);
    pl_add_rect_filled(gAppData.bgDrawLayer, (plVec2){10.0f, 100.0f}, (plVec2){10.0f + textSize0.x, 100.0f + textSize0.y}, (plVec4){0.0f, 0.0f, 0.2f, 0.5f});
    pl_add_rect_filled(gAppData.bgDrawLayer, (plVec2){10.0f, 10.0f}, (plVec2){10.0f + textSize1.x, 10.0f + textSize1.y}, (plVec4){0.0f, 0.0f, 0.2f, 0.5f});
    

    // submit draw layers
    pl_submit_draw_layer(gAppData.bgDrawLayer);
    pl_submit_draw_layer(gAppData.fgDrawLayer);

    // submit draw lists
    pl_submit_drawlist_metal(gAppData.drawlist, gAppData.viewportSize[0], gAppData.viewportSize[1], renderEncoder);

    // finish recording
    [renderEncoder endEncoding];

    // present
    [commandBuffer presentDrawable:currentDrawable];

    // submit command buffer
    [commandBuffer commit];
}