// This example requires a custom ffmpeg win64 build from https://ffmpeg.zeranoe.com/builds/
// Additional resources:
// http://dranger.com/ffmpeg/tutorial01.html
// https://blogs.gentoo.org/lu_zero/2015/10/15/deprecating-avpicture/
// https://blogs.gentoo.org/lu_zero/2016/03/29/new-avcodec-api/
// https://www.codeproject.com/tips/489450/creating-custom-ffmpeg-io-context

#define FPL_IMPLEMENTATION
#define FPL_AUTO_NAMESPACE
#include "final_platform_layer.hpp"

#include <assert.h> // assert
#include <Windows.h> // BITMAPINFOHEADER, BITMAPFILEHEADER

//
// FFMPEG headers and function prototypes
//
extern "C" {
#	include <libavcodec/avcodec.h>
#	include <libavformat/avformat.h>
#	include <libavutil/avutil.h>
#	include <libavutil/imgutils.h>
#	include <libswscale\swscale.h>
}

// av_register_all
#define FFMPEG_AV_REGISTER_ALL_FUNC(name) void name(void)
typedef FFMPEG_AV_REGISTER_ALL_FUNC(ffmpeg_av_register_all_func);
// avformat_close_input
#define FFMPEG_AVFORMAT_CLOSE_INPUT_FUNC(name) void name(AVFormatContext **s)
typedef FFMPEG_AVFORMAT_CLOSE_INPUT_FUNC(ffmpeg_avformat_close_input_func);
// avformat_open_input
#define FFMPEG_AVFORMAT_OPEN_INPUT_FUNC(name) int name(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options)
typedef FFMPEG_AVFORMAT_OPEN_INPUT_FUNC(ffmpeg_avformat_open_input_func);
// avformat_find_stream_info
#define FFMPEG_AVFORMAT_FIND_STREAM_INFO_FUNC(name) int name(AVFormatContext *ic, AVDictionary **options)
typedef FFMPEG_AVFORMAT_FIND_STREAM_INFO_FUNC(ffmpeg_avformat_find_stream_info_func);
// av_dump_format
#define FFMPEG_AV_DUMP_FORMAT_FUNC(name) void name(AVFormatContext *ic, int index, const char *url, int is_output)
typedef FFMPEG_AV_DUMP_FORMAT_FUNC(ffmpeg_av_dump_format_func);
// av_read_frame
#define FFMPEG_AV_READ_FRAME_FUNC(name) int name(AVFormatContext *s, AVPacket *pkt)
typedef FFMPEG_AV_READ_FRAME_FUNC(ffmpeg_av_read_frame_func);

// avcodec_free_context
#define FFMPEG_AVCODEC_FREE_CONTEXT_FUNC(name) void name(AVCodecContext **avctx)
typedef FFMPEG_AVCODEC_FREE_CONTEXT_FUNC(ffmpeg_avcodec_free_context_func);
// avcodec_alloc_context3
#define FFMPEG_AVCODEC_ALLOC_CONTEXT3_FUNC(name) AVCodecContext *name(const AVCodec *codec)
typedef FFMPEG_AVCODEC_ALLOC_CONTEXT3_FUNC(ffmpeg_avcodec_alloc_context3_func);
// avcodec_parameters_to_context
#define FFMPEG_AVCODEC_PARAMETERS_TO_CONTEXT_FUNC(name) int name(AVCodecContext *codec, const AVCodecParameters *par)
typedef FFMPEG_AVCODEC_PARAMETERS_TO_CONTEXT_FUNC(ffmpeg_avcodec_parameters_to_context_func);
// avcodec_find_decoder
#define FFMPEG_AVCODEC_FIND_DECODER_FUNC(name) AVCodec *name(enum AVCodecID id)
typedef FFMPEG_AVCODEC_FIND_DECODER_FUNC(ffmpeg_avcodec_find_decoder_func);
// avcodec_open2
#define FFMPEG_AVCODEC_OPEN2_FUNC(name) int name(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options)
typedef FFMPEG_AVCODEC_OPEN2_FUNC(ffmpeg_avcodec_open2_func);
// av_packet_unref
#define FFMPEG_AV_PACKET_UNREF_FUNC(name) void name(AVPacket *pkt)
typedef FFMPEG_AV_PACKET_UNREF_FUNC(ffmpeg_av_packet_unref_func);
// avcodec_receive_frame
#define FFMPEG_AVCODEC_RECEIVE_FRAME_FUNC(name) int name(AVCodecContext *avctx, AVFrame *frame)
typedef FFMPEG_AVCODEC_RECEIVE_FRAME_FUNC(ffmpeg_avcodec_receive_frame_func);
// avcodec_send_packet
#define FFMPEG_AVCODEC_SEND_PACKET_FUNC(name) int name(AVCodecContext *avctx, const AVPacket *avpkt)
typedef FFMPEG_AVCODEC_SEND_PACKET_FUNC(ffmpeg_avcodec_send_packet_func);

// av_frame_alloc
#define FFMPEG_AV_FRAME_ALLOC_FUNC(name) AVFrame *name(void)
typedef FFMPEG_AV_FRAME_ALLOC_FUNC(ffmpeg_av_frame_alloc_func);
// av_frame_free
#define FFMPEG_AV_FRAME_FREE_FUNC(name) void name(AVFrame **frame)
typedef FFMPEG_AV_FRAME_FREE_FUNC(ffmpeg_av_frame_free_func);
// av_image_get_buffer_size
#define FFMPEG_AV_IMAGE_GET_BUFFER_SIZE_FUNC(name) int name(enum AVPixelFormat pix_fmt, int width, int height, int align)
typedef FFMPEG_AV_IMAGE_GET_BUFFER_SIZE_FUNC(ffmpeg_av_image_get_buffer_size_func);
// av_image_get_linesize
#define FFMPEG_AV_IMAGE_GET_LINESIZE_FUNC(name) int name(enum AVPixelFormat pix_fmt, int width, int plane)
typedef FFMPEG_AV_IMAGE_GET_LINESIZE_FUNC(ffmpeg_av_image_get_linesize_func);
// av_image_fill_arrays
#define FFMPEG_AV_IMAGE_FILL_ARRAYS_FUNC(name) int name(uint8_t *dst_data[4], int dst_linesize[4], const uint8_t *src, enum AVPixelFormat pix_fmt, int width, int height, int align)
typedef FFMPEG_AV_IMAGE_FILL_ARRAYS_FUNC(ffmpeg_av_image_fill_arrays_func);

// sws_getContext
#define FFMPEG_SWS_GET_CONTEXT_FUNC(name) struct SwsContext *name(int srcW, int srcH, enum AVPixelFormat srcFormat, int dstW, int dstH, enum AVPixelFormat dstFormat, int flags, SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param)
typedef FFMPEG_SWS_GET_CONTEXT_FUNC(ffmpeg_sws_getContext_func);
// sws_scale
#define FFMPEG_SWS_SCALE_FUNC(name) int name(struct SwsContext *c, const uint8_t *const srcSlice[], const int srcStride[], int srcSliceY, int srcSliceH, uint8_t *const dst[], const int dstStride[])
typedef FFMPEG_SWS_SCALE_FUNC(ffmpeg_sws_scale_func);
// sws_freeContext
#define FFMPEG_SWS_FREE_CONTEXT_FUNC(name) void name(struct SwsContext *swsContext)
typedef FFMPEG_SWS_FREE_CONTEXT_FUNC(ffmpeg_sws_freeContext_func);

#define FFMPEG_GET_FUNCTION_ADDRESS(libHandle, libName, target, type, name) \
	target = (type *)GetDynamicLibraryProc(libHandle, name); \
	if (target == nullptr) { \
		ConsoleFormatError("[FFMPEG] Failed getting '%s' from library '%s'!", name, libName); \
		goto release; \
	}

struct ffmpegFunctions {
	// Format
	ffmpeg_av_register_all_func *avRegisterAll;
	ffmpeg_avformat_close_input_func *avFormatCloseInput;
	ffmpeg_avformat_open_input_func *avFormatOpenInput;
	ffmpeg_avformat_find_stream_info_func *avFormatFindStreamInfo;
	ffmpeg_av_dump_format_func *avDumpFormat;
	ffmpeg_av_read_frame_func *avReadFrame;

	// Codec
	ffmpeg_avcodec_free_context_func *avcodecFreeContext;
	ffmpeg_avcodec_alloc_context3_func *avcodecAllocContext3;
	ffmpeg_avcodec_parameters_to_context_func *avcodecParametersToContext;
	ffmpeg_avcodec_find_decoder_func *avcodecFindDecoder;
	ffmpeg_avcodec_open2_func *avcodecOpen2;
	ffmpeg_av_packet_unref_func *avPacketUnref;
	ffmpeg_avcodec_receive_frame_func *avcodecReceiveFrame;
	ffmpeg_avcodec_send_packet_func *avcodecSendPacket;

	// Util
	ffmpeg_av_frame_alloc_func *avFrameAlloc;
	ffmpeg_av_frame_free_func *avFrameFree;
	ffmpeg_av_image_get_buffer_size_func *avImageGetBufferSize;
	ffmpeg_av_image_get_linesize_func *avImageGetLineSize;
	ffmpeg_av_image_fill_arrays_func *avImageFillArrays;

	// SWS
	ffmpeg_sws_getContext_func *swsGetContext;
	ffmpeg_sws_scale_func *swsScale;
	ffmpeg_sws_freeContext_func *swsFreeContext;
};

static ffmpegFunctions *globalFFMPEGFunctions = nullptr;

static int DecodeVideoPacket(struct AVCodecContext *avctx, struct AVFrame *frame, struct AVPacket *pkt, bool *got_frame) {
	ffmpegFunctions &ffmpeg = *globalFFMPEGFunctions;

	int ret;

	*got_frame = false;

	if (pkt) {
		// @NOTE(final): I have no idea why i need to "send" the packet -> "push" would be more approriate here.
		ret = ffmpeg.avcodecSendPacket(avctx, pkt);
		if (ret < 0) {
			return ret == AVERROR_EOF ? 0 : ret;
		}
	}

	// @NOTE(final): We "pushed" all required packets and are able to decode the actual frame
	ret = ffmpeg.avcodecReceiveFrame(avctx, frame);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		return ret;
	}

	if (ret >= 0) {
		*got_frame = true;
	}

	return 0;
}

static void SaveBitmapRGB24(uint8_t *source, uint32_t width, uint32_t height, uint32_t scanline, const char *targetFilePath) {
	assert(scanline == (width * 3));

	BITMAPINFOHEADER bih = {};
	bih.biBitCount = 24;
	bih.biClrImportant = 0;
	bih.biCompression = BI_RGB;
	bih.biHeight = -(LONG)height;
	bih.biWidth = width;
	bih.biPlanes = 1;
	bih.biSizeImage = DWORD(scanline * height);
	bih.biSize = sizeof(BITMAPINFOHEADER);

	BITMAPFILEHEADER bfh = {};
	bfh.bfType = ((WORD)('M' << 8) | 'B');
	bfh.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + bih.biSize + bih.biSizeImage);
	bfh.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + bih.biSize);

	FileHandle handle = CreateBinaryFile(targetFilePath);
	if (handle.isValid) {
		WriteFileBlock32(handle, &bfh, sizeof(BITMAPFILEHEADER));
		WriteFileBlock32(handle, &bih, sizeof(BITMAPINFOHEADER));
		WriteFileBlock32(handle, source, bih.biSizeImage);
		CloseFile(handle);
	}
}

struct FFMpegState {
	AVFormatContext *formatCtx;
	AVCodecContext *videoCtx;
	AVCodec *videoCodec;
	AVFrame *sourceNativeFrame;
	AVFrame *targetRGBFrame;
	uint8_t *targetRGBBuffer;
	SwsContext *softwareCtx;
};

static void ConvertRGB24ToBackBuffer(VideoBackBuffer *backbuffer, int width, int height, int sourceScanLine, uint8_t *sourceData) {
	for (int y = 0; y < height; ++y) {
		uint8_t *src = sourceData + y * sourceScanLine;
		int invertY = height - 1 - y;
		uint32_t *dst = (uint32_t *)((uint8_t *)backbuffer->pixels + invertY * backbuffer->stride);
		for (int x = 0; x < width; ++x) {
			uint8_t r = *src++;
			uint8_t g = *src++;
			uint8_t b = *src++;
			uint8_t a = 255;
			*dst++ = (a << 24) | (b << 16) | (g << 8) | r;
		}
	}
}

int main(int argc, char **argv) {
	int result = 0;
	Settings settings = DefaultSettings();
	settings.video.driverType = VideoDriverType::Software;
	settings.video.isAutoSize = false;
	if (InitPlatform(InitFlags::Window, settings)) {
		globalFFMPEGFunctions = (ffmpegFunctions *)MemoryAlignedAllocate(sizeof(ffmpegFunctions), 16);

		ffmpegFunctions &ffmpeg = *globalFFMPEGFunctions;

		FFMpegState state = {};

		//
		// Load ffmpeg libraries
		//
		const char *avFormatLibFile = "avformat-58.dll";
		const char *avCodecLibFile = "avcodec-58.dll";
		const char *avUtilLibFile = "avutil-56.dll";
		const char *swsScaleLibFile = "swscale-5.dll";
		DynamicLibraryHandle avFormatLib = DynamicLibraryLoad(avFormatLibFile);
		DynamicLibraryHandle avCodecLib = DynamicLibraryLoad(avCodecLibFile);
		DynamicLibraryHandle avUtilLib = DynamicLibraryLoad(avUtilLibFile);
		DynamicLibraryHandle swsScaleLib = DynamicLibraryLoad(swsScaleLibFile);
		FFMPEG_GET_FUNCTION_ADDRESS(avFormatLib, avFormatLibFile, ffmpeg.avRegisterAll, ffmpeg_av_register_all_func, "av_register_all");
		FFMPEG_GET_FUNCTION_ADDRESS(avFormatLib, avFormatLibFile, ffmpeg.avFormatCloseInput, ffmpeg_avformat_close_input_func, "avformat_close_input");
		FFMPEG_GET_FUNCTION_ADDRESS(avFormatLib, avFormatLibFile, ffmpeg.avFormatOpenInput, ffmpeg_avformat_open_input_func, "avformat_open_input");
		FFMPEG_GET_FUNCTION_ADDRESS(avFormatLib, avFormatLibFile, ffmpeg.avFormatFindStreamInfo, ffmpeg_avformat_find_stream_info_func, "avformat_find_stream_info");
		FFMPEG_GET_FUNCTION_ADDRESS(avFormatLib, avFormatLibFile, ffmpeg.avDumpFormat, ffmpeg_av_dump_format_func, "av_dump_format");
		FFMPEG_GET_FUNCTION_ADDRESS(avFormatLib, avFormatLibFile, ffmpeg.avReadFrame, ffmpeg_av_read_frame_func, "av_read_frame");

		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avcodecFreeContext, ffmpeg_avcodec_free_context_func, "avcodec_free_context");
		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avcodecAllocContext3, ffmpeg_avcodec_alloc_context3_func, "avcodec_alloc_context3");
		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avcodecParametersToContext, ffmpeg_avcodec_parameters_to_context_func, "avcodec_parameters_to_context");
		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avcodecFindDecoder, ffmpeg_avcodec_find_decoder_func, "avcodec_find_decoder");
		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avcodecOpen2, ffmpeg_avcodec_open2_func, "avcodec_open2");
		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avPacketUnref, ffmpeg_av_packet_unref_func, "av_packet_unref");
		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avcodecReceiveFrame, ffmpeg_avcodec_receive_frame_func, "avcodec_receive_frame");
		FFMPEG_GET_FUNCTION_ADDRESS(avCodecLib, avCodecLibFile, ffmpeg.avcodecSendPacket, ffmpeg_avcodec_send_packet_func, "avcodec_send_packet");

		FFMPEG_GET_FUNCTION_ADDRESS(avUtilLib, avUtilLibFile, ffmpeg.avFrameAlloc, ffmpeg_av_frame_alloc_func, "av_frame_alloc");
		FFMPEG_GET_FUNCTION_ADDRESS(avUtilLib, avUtilLibFile, ffmpeg.avFrameFree, ffmpeg_av_frame_free_func, "av_frame_free");
		FFMPEG_GET_FUNCTION_ADDRESS(avUtilLib, avUtilLibFile, ffmpeg.avImageGetBufferSize, ffmpeg_av_image_get_buffer_size_func, "av_image_get_buffer_size");
		FFMPEG_GET_FUNCTION_ADDRESS(avUtilLib, avUtilLibFile, ffmpeg.avImageGetLineSize, ffmpeg_av_image_get_linesize_func, "av_image_get_linesize");
		FFMPEG_GET_FUNCTION_ADDRESS(avUtilLib, avUtilLibFile, ffmpeg.avImageFillArrays, ffmpeg_av_image_fill_arrays_func, "av_image_fill_arrays");

		FFMPEG_GET_FUNCTION_ADDRESS(swsScaleLib, swsScaleLibFile, ffmpeg.swsGetContext, ffmpeg_sws_getContext_func, "sws_getContext");
		FFMPEG_GET_FUNCTION_ADDRESS(swsScaleLib, swsScaleLibFile, ffmpeg.swsScale, ffmpeg_sws_scale_func, "sws_scale");
		FFMPEG_GET_FUNCTION_ADDRESS(swsScaleLib, swsScaleLibFile, ffmpeg.swsFreeContext, ffmpeg_sws_freeContext_func, "sws_freeContext");

		// Get home path
		char homepathBuffer[512];
		char *homePath = GetHomePath(homepathBuffer, FPL_ARRAYCOUNT(homepathBuffer));

		// Build output images path and enforce directory
		char outputImagesPathBuffer[512];
		char *outputImagesPath = CombinePath(outputImagesPathBuffer, FPL_ARRAYCOUNT(outputImagesPathBuffer), 2, homePath, "FPL_TempImages");
		files::CreateDirectories(outputImagesPath);

		// Example video: /home/[user]/Videos/Testvideos/Kayaking.mp4
		char movieFilePathBuffer[512];
		char *mediaFilePath = CombinePath(movieFilePathBuffer, FPL_ARRAYCOUNT(homepathBuffer), 4, homePath, "Videos", "Testvideos", "Kayaking.mp4");

		// Register all formats and codecs
		ffmpeg.avRegisterAll();

		// @TODO(final): Custom IO!

		// Open video file
		if (ffmpeg.avFormatOpenInput(&state.formatCtx, mediaFilePath, nullptr, nullptr) != 0) {
			ConsoleFormatError("Failed opening media file '%s'!\n", mediaFilePath);
			goto release;
		}
		// Retrieve stream information
		if (ffmpeg.avFormatFindStreamInfo(state.formatCtx, nullptr) < 0) {
			ConsoleFormatError("Failed getting stream informations for media file '%s'!\n", mediaFilePath);
			goto release;
		}

		// Dump information about file onto standard error
		ffmpeg.avDumpFormat(state.formatCtx, 0, mediaFilePath, 0);

		// Find the first video stream
		int videoStream = -1;
		for (uint32_t steamIndex = 0; steamIndex < state.formatCtx->nb_streams; steamIndex++) {
			if (state.formatCtx->streams[steamIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				videoStream = steamIndex;
				break;
			}
		}
		if (videoStream == -1) {
			ConsoleFormatError("No video stream in media file '%s' found!\n", mediaFilePath);
			goto release;
		}

		// Get a pointer to the video stream
		AVStream *pVideoStream = state.formatCtx->streams[videoStream];
		assert(pVideoStream->codecpar != nullptr);

		// Get codec name
		char codecName[5] = {};
		MemoryCopy(&pVideoStream->codecpar->codec_tag, 4, codecName);

		// Create video context
		state.videoCtx = ffmpeg.avcodecAllocContext3(nullptr);
		if (ffmpeg.avcodecParametersToContext(state.videoCtx, pVideoStream->codecpar) < 0) {
			ConsoleFormatError("Failed getting video context from codec '%s' in media file '%s'!\n", codecName, mediaFilePath);
			goto release;
		}

		// Find video decoder
		state.videoCodec = ffmpeg.avcodecFindDecoder(pVideoStream->codecpar->codec_id);
		if (state.videoCodec == nullptr) {
			ConsoleFormatError("Unsupported video codec '%s' in media file '%s' found!\n", codecName, mediaFilePath);
			goto release;
		}

		// Open codec
		if (ffmpeg.avcodecOpen2(state.videoCtx, state.videoCodec, nullptr) < 0) {
			ConsoleFormatError("Failed opening video codec '%s' from media file '%s'!\n", codecName, mediaFilePath);
			goto release;
		}

		// Allocate native video frame
		state.sourceNativeFrame = ffmpeg.avFrameAlloc();
		if (state.sourceNativeFrame == nullptr) {
			ConsoleFormatError("Failed allocating native video frame for media file '%s'!\n", mediaFilePath);
			goto release;
		}

		// Allocate RGB video frame
		state.targetRGBFrame = ffmpeg.avFrameAlloc();
		if (state.targetRGBFrame == nullptr) {
			ConsoleFormatError("Failed allocating RGB video frame for media file '%s'!\n", mediaFilePath);
			goto release;
		}

		// Allocate RGB buffer
		AVPixelFormat targetPixelFormat = AVPixelFormat::AV_PIX_FMT_BGR24;
		size_t rgbFrameSize = ffmpeg.avImageGetBufferSize(targetPixelFormat, state.videoCtx->width, state.videoCtx->height, 1);
		state.targetRGBBuffer = (uint8_t *)MemoryAlignedAllocate(rgbFrameSize, 16);

		// Setup RGB video frame and give it access to the actual data
		ffmpeg.avImageFillArrays(state.targetRGBFrame->data, state.targetRGBFrame->linesize, state.targetRGBBuffer, targetPixelFormat, state.videoCtx->width, state.videoCtx->height, 1);

		// Get software context
		state.softwareCtx = ffmpeg.swsGetContext(
			state.videoCtx->width,
			state.videoCtx->height,
			state.videoCtx->pix_fmt,
			state.videoCtx->width,
			state.videoCtx->height,
			targetPixelFormat,
			SWS_BILINEAR,
			nullptr,
			nullptr,
			nullptr
		);

		// Resize backbuffer
		ResizeVideoBackBuffer(state.videoCtx->width, state.videoCtx->height);
		VideoBackBuffer *backBuffer = GetVideoBackBuffer();

		//
		// App loop
		//
		while (WindowUpdate()) {
			// @TODO(final): For now we read one packet for each window update. This is really slow, but works for the time being. Multithread it!
			{
				AVPacket packet;
				if (ffmpeg.avReadFrame(state.formatCtx, &packet) >= 0) {
					if (packet.stream_index == videoStream) {
						bool frameDecoded = false;
						DecodeVideoPacket(state.videoCtx, state.sourceNativeFrame, &packet, &frameDecoded);
						if (frameDecoded) {
							// @TODO(final): Decode picture format directly into the backbuffer, without the software scaling!

							// Convert native frame to target RGB24 frame
							ffmpeg.swsScale(state.softwareCtx, (uint8_t const * const *)state.sourceNativeFrame->data, state.sourceNativeFrame->linesize, 0, state.videoCtx->height, state.targetRGBFrame->data, state.targetRGBFrame->linesize);
							// Convert RGB24 frame to RGB32 backbuffer
							ConvertRGB24ToBackBuffer(backBuffer, state.videoCtx->width, state.videoCtx->height, *state.targetRGBFrame->linesize, state.targetRGBBuffer);
						}
					}
				}
			}


			WindowFlip();
		}

	release:
		// Release media
		if (state.softwareCtx != nullptr) {
			ffmpeg.swsFreeContext(state.softwareCtx);
		}
		if (state.targetRGBBuffer != nullptr) {
			MemoryAlignedFree(state.targetRGBBuffer);
		}
		if (state.targetRGBFrame != nullptr) {
			ffmpeg.avFrameFree(&state.targetRGBFrame);
		}
		if (state.sourceNativeFrame != nullptr) {
			ffmpeg.avFrameFree(&state.sourceNativeFrame);
		}
		if (state.videoCtx != nullptr) {
			ffmpeg.avcodecFreeContext(&state.videoCtx);
		}
		if (state.formatCtx != nullptr) {
			ffmpeg.avFormatCloseInput(&state.formatCtx);
		}

		// Release FFMPEG
		DynamicLibraryUnload(avUtilLib);
		DynamicLibraryUnload(avCodecLib);
		DynamicLibraryUnload(avFormatLib);
		MemoryAlignedFree(globalFFMPEGFunctions);

		ReleasePlatform();

		result = 0;
	} else {
		result = -1;
	}

	return(result);
}