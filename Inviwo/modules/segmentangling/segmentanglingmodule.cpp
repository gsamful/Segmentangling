#include <modules/segmentangling/segmentanglingmodule.h>

#include <modules/opengl/shader/shadermanager.h>
#include <modules/segmentangling/processors/contourfilter.h>
#include <modules/segmentangling/processors/loadcontourtree.h>
#include <modules/segmentangling/processors/segmentationidraycaster.h>

namespace inviwo {

SegmentanglingModule::SegmentanglingModule(InviwoApplication* app)
    : InviwoModule(app, "Segmentangling")
{
    ShaderManager::getPtr()->addShaderSearchPath(getPath(ModulePath::GLSL));

    registerPort<DataOutport<GLuint>>("bufferOutport");
    registerPort<DataInport<GLuint>>("bufferInport");

    registerProcessor<ContourFilter>();
    registerProcessor<LoadContourTree>();
    registerProcessor<SegmentationIdRaycaster>();
}

}  // namespace
