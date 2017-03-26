
#include "stdafx.h"
#include "CScreenDXGrabber.h"
#include <MMSystem.h>

namespace ScreenGrabber
{
	CScreenDXGrabber::CScreenDXGrabber()
	{
		frame_rate_ = 15;
		grab_rect_.left = grab_rect_.top = 0;
		grab_rect_.right = GetSystemMetrics(SM_CXSCREEN);
		grab_rect_.bottom = GetSystemMetrics(SM_CYSCREEN);
		started_ = false;
		last_tick_count_ = 0;
		perf_freq_.QuadPart = 0;
	}

	CScreenDXGrabber::~CScreenDXGrabber()
	{
		if (started_)
		{
			StopGrab();
		}
	}

	void CScreenDXGrabber::RegisterDataCb(IScreenGrabberDataCb* cb)
	{
		vec_data_cb_.push_back(cb);
	}

	void CScreenDXGrabber::UnRegisterDataCb(IScreenGrabberDataCb* cb)
	{
		auto iter = std::find(vec_data_cb_.begin(), vec_data_cb_.end(), cb);
		if (iter != vec_data_cb_.end())
		{
			vec_data_cb_.erase(iter);
		}
	}

	void CScreenDXGrabber::SetGrabRect(int left, int top, int right, int bottom)
	{
		grab_rect_.left = left;
		grab_rect_.top = top;
		grab_rect_.right = right;
		grab_rect_.bottom = bottom;
	}

	void CScreenDXGrabber::SetGrabFrameRate(int frame_rate)
	{
		frame_rate_ = frame_rate;
	}

	bool CScreenDXGrabber::StartGrab()
	{
		if (started_)
		{
			OutputDebugStringA("Already started \n");
			return false;
		}

		QueryPerformanceFrequency(&perf_freq_);
		frame_interval_tick_ = perf_freq_.QuadPart / frame_rate_;

		if (!InitD3D())
		{
			OutputDebugStringA("InitD3D failed \n");
			return false;
		}

		StartGrabThread();
		started_ = true;
		return true;
	}


	void CScreenDXGrabber::StopGrab()
	{
		if (started_)
		{
			StopGrabThread();
			UnInitD3D();
			started_ = false;
		}
	}
	bool CScreenDXGrabber::InitD3D()
	{
		D3DDISPLAYMODE ddm;
		D3DPRESENT_PARAMETERS d3dpp;
		d3d9_ptr_ = Direct3DCreate9(D3D_SDK_VERSION);
		if (d3d9_ptr_ == NULL)
		{
			OutputDebugStringA("Create d3d9 failed\n");
			return false;
		}

		if (FAILED(d3d9_ptr_->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm)))
		{
			OutputDebugStringA("GetAdapterDisplayMode failed");
			return false;
		}

		ZeroMemory(&d3dpp, sizeof(D3DPRESENT_PARAMETERS));

		d3dpp.Windowed = true;
		d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		d3dpp.BackBufferFormat = ddm.Format;
		d3dpp.BackBufferHeight = ddm.Height;
		d3dpp.BackBufferWidth = ddm.Width;
		d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = NULL;
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		if (FAILED(d3d9_ptr_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d_dev_ptr_)))
		{
			OutputDebugStringA("CreateDevice failed \n");
			return false;
		}

		if (FAILED(d3d_dev_ptr_->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &d3d_surface_ptr_, NULL)))
		{
			OutputDebugStringA("CreateOffscreenPlainSurface failed");
			return false;
		}

		return true;

	}

	void CScreenDXGrabber::UnInitD3D()
	{
		if (d3d_surface_ptr_)
		{
			d3d_surface_ptr_->Release();
			d3d_surface_ptr_ = NULL;
		}
		if (d3d_dev_ptr_)
		{
			d3d_dev_ptr_->Release();
			d3d_dev_ptr_ = NULL;
		}
		if (d3d9_ptr_)
		{
			d3d9_ptr_->Release();
			d3d9_ptr_ = NULL;
		}
	}


	void CScreenDXGrabber::StartGrabThread()
	{
		run_ = true;
		grab_thread_.swap(std::thread(std::bind(&CScreenDXGrabber::GrabThreadProc, this)));
		SetThreadPriority(grab_thread_.native_handle(), THREAD_PRIORITY_TIME_CRITICAL);

	}

	void CScreenDXGrabber::StopGrabThread()
	{
		run_ = false;
		if (grab_thread_.joinable())
			grab_thread_.join();
	}

	void CScreenDXGrabber::GrabThreadProc()
	{
		while (run_)
		{
			int64_t begin_time = timeGetTime();
			d3d_dev_ptr_->GetFrontBufferData(0, d3d_surface_ptr_);
			D3DLOCKED_RECT lockedRect;
			d3d_surface_ptr_->LockRect(&lockedRect,  &grab_rect_, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
			for (IScreenGrabberDataCb* cb : vec_data_cb_)
			{
				cb->OnScreenData(lockedRect.pBits, grab_rect_.right - grab_rect_.left, 
					grab_rect_.bottom - grab_rect_.top, ScreenGrabber::PIX_FMT::PIX_FMT_BGRA);
			}
			d3d_surface_ptr_->UnlockRect();

			int64_t duration = timeGetTime() - begin_time;
			char log[128] = { 0 };
			sprintf(log, "DX capture interval: %lld \n", duration);
			OutputDebugStringA(log);
		}
	}


}

