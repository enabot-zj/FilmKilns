/*
* Copyright (c) 2018-present, aliminabc@gmail.com.
*
* This source code is licensed under the MIT license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "AlImage.h"
#include "HwBitmapFactory.h"
#include "HwTexture.h"
#include "ObjectBox.h"

AlImage::AlImage(string alias) : Unit(alias) {
    registerEvent(EVENT_COMMON_PREPARE, reinterpret_cast<EventFunc>(&AlImage::onPrepare));
    registerEvent(EVENT_COMMON_INVALIDATE, reinterpret_cast<EventFunc>(&AlImage::onInvalidate));
    registerEvent(EVENT_AIMAGE_NEW_LAYER, reinterpret_cast<EventFunc>(&AlImage::onNewLayer));
    registerEvent(EVENT_AIMAGE_UPDATE_CANVAS,
                  reinterpret_cast<EventFunc>(&AlImage::onUpdateCanvas));
}

AlImage::~AlImage() {

}

bool AlImage::eventRelease(Message *msg) {
    mCanvas.release();
    mLayerManager.release();
    if (texAllocator) {
        delete texAllocator;
        texAllocator = nullptr;
    }
    return true;
}

bool AlImage::onPrepare(Message *msg) {
    texAllocator = new TextureAllocator();
    return true;
}

bool AlImage::onUpdateCanvas(Message *m) {
    auto model = getCanvas();
    mCanvas.update(model->getWidth(), model->getHeight(), model->getColor(), texAllocator);
    onInvalidate(nullptr);
    return true;
}

bool AlImage::onNewLayer(Message *msg) {
    mLayerManager.update(getLayers(), texAllocator);
    _newDefaultCanvas();
    onInvalidate(nullptr);
    return true;
}

bool AlImage::onInvalidate(Message *m) {
    if (mLayerManager.empty()) {
        return true;
    }
    mCanvas.clear();
    auto layer = mLayerManager.getLayout(0);
    layer->draw(&mCanvas);
    Message *msg = new Message(EVENT_RENDER_FILTER, nullptr);
    auto tex = mCanvas.getOutput();
    if (nullptr == tex) {
        Logcat::e("AlImage", "%s(%d): Empty canvas", __FUNCTION__, __LINE__);
        return true;
    }
    msg->obj = HwTexture::wrap(tex->target(), tex->texId(),
                               tex->getWidth(),
                               tex->getHeight(),
                               tex->fmt());
    postEvent(msg);
    return true;
}

/*+------------------+*/
/*|     Model        |*/
/*+------------------+*/
AlImageCanvasModel *AlImage::getCanvas() {
    return dynamic_cast<AlImageCanvasModel *>(getObject("canvas"));
}

std::list<AlImageLayerModel *> *AlImage::getLayers() {
    auto *obj = static_cast<ObjectBox *>(getObject("layers"));
    return static_cast<list<AlImageLayerModel *> *>(obj->ptr);
}

void AlImage::_newDefaultCanvas() {
    if (0 == mLayerManager.size()) {
        return;
    }
    auto model = getCanvas();
    if (model->getWidth() > 0 && model->getHeight() > 0) {
        return;
    }
    auto layer = mLayerManager.getLayout(0);
    model->set(layer->getWidth(), layer->getHeight(), 0);
    mCanvas.update(model->getWidth(), model->getHeight(), model->getColor(), texAllocator);
}