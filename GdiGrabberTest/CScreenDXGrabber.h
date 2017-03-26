#ifndef CSCREENDXGRABBER_H
#define CSCREENDXGRABBER_H

#include "IScreenGrabber.h"
#include <vector>
#include <windows.h>
#include <atomic>
#include <thread>
#include <d3d9.h>

namespace ScreenGrabber
{
	class CScreenDXGrabber : public IScreenGrabber
	{
	public:
		CScreenDXGrabber();
		~CScreenDXGrabber();

		void RegisterDataCb(IScreenGrabberDataCb* cb) override;
		void UnRegisterDataCb(IScreenGrabberDataCb* cb) override;

		void SetGrabRect(int left, int top, int right, int bottom) override;
		void SetGrabFrameRate(int frame_rate) override;

		bool StartGrab() override;
		void StopGrab() override;

	private:

		bool InitD3D();
		void UnInitD3D();
		void StartGrabThread();
		void StopGrabThread();
		void GrabThreadProc();

	private:
		std::vector<IScreenGrabberDataCb*> vec_data_cb_;
		int screen_width_;
		int screen_height_;
		RECT grab_rect_;
		int frame_rate_;

		bool started_;
		std::atomic_bool run_;
		std::thread grab_thread_;

		LARGE_INTEGER perf_freq_;
		int64_t last_tick_count_;
		int64_t frame_interval_tick_;

		HANDLE m_hGrabTimer;

		IDirect3D9* d3d9_ptr_;
		IDirect3DDevice9* d3d_dev_ptr_;
		IDirect3DSurface9* d3d_surface_ptr_;

	};

}

#endif