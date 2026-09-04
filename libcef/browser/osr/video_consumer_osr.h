#ifndef LIBCEF_BROWSER_OSR_VIDEO_CONSUMER_OSR_H_
#define LIBCEF_BROWSER_OSR_VIDEO_CONSUMER_OSR_H_

#include <map>
#include <memory>
#include <optional>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "components/viz/host/client_frame_sink_video_capturer.h"
#include "media/capture/mojom/video_capture_types.mojom.h"

#if BUILDFLAG(IS_WIN)
#include "ui/gfx/gpu_memory_buffer_handle.h"
#endif

class CefRenderWidgetHostViewOSR;

// Holds a captured frame's pool slot and buffer handle for as long as the
// client is using the surface. Defined in the implementation file.
class CefCapturedFrameLease;

class CefVideoConsumerOSR : public viz::mojom::FrameSinkVideoConsumer {
 public:
  CefVideoConsumerOSR(CefRenderWidgetHostViewOSR* view,
                      bool use_shared_texture);

  CefVideoConsumerOSR(const CefVideoConsumerOSR&) = delete;
  CefVideoConsumerOSR& operator=(const CefVideoConsumerOSR&) = delete;

  ~CefVideoConsumerOSR() override;

  void SetActive(bool active);
  void SetFrameRate(base::TimeDelta frame_rate);
  void SizeChanged(const gfx::Size& size_in_pixels);
  void RequestRefreshFrame(const std::optional<gfx::Rect>& bounds_in_pixels);

  // Release a surface leased to the client, returning the frame to the capture
  // pool. Unknown or already released ids are ignored.
  void ReleaseSurface(uint64_t surface_id);

 private:
  // viz::mojom::FrameSinkVideoConsumer implementation.
  void OnFrameCaptured(
      media::mojom::VideoBufferHandlePtr data,
      media::mojom::VideoFrameInfoPtr info,
      const gfx::Rect& content_rect,
      mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
          callbacks) override;
  void OnFrameWithEmptyRegionCapture() override {}
  void OnStopped() override {}
  void OnLog(const std::string& message) override {}
  void OnNewCaptureVersion(
      const media::CaptureVersion& capture_version) override {}

  const bool use_shared_texture_;

  const raw_ptr<CefRenderWidgetHostViewOSR> view_;
  std::unique_ptr<viz::ClientFrameSinkVideoCapturer> video_capturer_;

  gfx::Size size_in_pixels_;
  std::optional<gfx::Rect> bounds_in_pixels_;

  // Surfaces currently leased to the client, keyed by the id handed to it.
  // Touched only on the thread that runs OnFrameCaptured and the release call,
  // so it needs no lock.
  std::map<uint64_t, std::unique_ptr<CefCapturedFrameLease>> leases_;
  uint64_t next_surface_id_ = 1;

#if BUILDFLAG(IS_WIN)
  // Stable identifiers for the capture pool's surfaces, assigned on first
  // sight and kept for the life of the consumer.
  //
  // Keyed on the DXGI handle's token rather than on the handle itself, because
  // the handle is duplicated per delivery and its value therefore differs
  // between two paints of the same surface. gfx::DXGIHandle says so directly:
  // the token is preserved across that duplication and is what callers should
  // compare. Mapped to a small dense integer so Chromium's types stay out of
  // the public structure.
  //
  // Bounded by the pool, which holds eleven frames. A resize builds a new pool
  // and leaves the old entries behind; they are integers, and the consumer is
  // recreated often enough that trimming them would cost more than it saves.
  std::map<gfx::DXGIHandleToken, uint64_t> pool_surface_ids_;
  uint64_t next_pool_surface_id_ = 1;
#endif
};

#endif  // LIBCEF_BROWSER_OSR_VIDEO_CONSUMER_OSR_H_
