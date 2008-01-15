/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_proto.h,v 1.11 2004/03/20 01:52:16 mvojkovi Exp $ */

#ifndef __NV_PROTO_H__
#define __NV_PROTO_H__

/* in nv_accel_common.c */
Bool NVAccelCommonInit(ScrnInfoPtr pScrn);
Bool NVAccelGetCtxSurf2DFormatFromPixmap(PixmapPtr pPix, int *fmt_ret);
Bool NVAccelGetCtxSurf2DFormatFromPicture(PicturePtr pPix, int *fmt_ret);

/* in nv_driver.c */
Bool   NVSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
void   NVAdjustFrame(int scrnIndex, int x, int y, int flags);
Bool   NVI2CInit(ScrnInfoPtr pScrn);
Bool NVMatchModePrivate(DisplayModePtr mode, uint32_t flags);

/* in nv_dri.c */
Bool NVDRIScreenInit(ScrnInfoPtr pScrn);
Bool NVDRIFinishScreenInit(ScrnInfoPtr pScrn);
extern const char *drmSymbols[], *driSymbols[];
Bool NVDRIGetVersion(ScrnInfoPtr pScrn);

/* in nv_dac.c */
Bool   NVDACInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
void   NVDACSave(ScrnInfoPtr pScrn, vgaRegPtr vgaReg,
                 NVRegPtr nvReg, Bool saveFonts);
void   NVDACRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg,
                    NVRegPtr nvReg, Bool restoreFonts);
void   NVDACLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
                        LOCO *colors, VisualPtr pVisual );
Bool   NVDACi2cInit(ScrnInfoPtr pScrn);

/* in nv_video.c */
void NVInitVideo(ScreenPtr);
void NVWaitVSync(ScrnInfoPtr pScrn, int crtc);
void NVSetPortDefaults (ScrnInfoPtr pScrn, NVPortPrivPtr pPriv);
unsigned int nv_window_belongs_to_crtc(ScrnInfoPtr, int, int, int, int);

/* in nv_setup.c */
void   RivaEnterLeave(ScrnInfoPtr pScrn, Bool enter);
void   NVCommonSetup(ScrnInfoPtr pScrn);

/* in nv_cursor.c */
Bool   NVCursorInit(ScreenPtr pScreen);
Bool NVCursorInitRandr12(ScreenPtr pScreen);
void nv_crtc_show_cursor(xf86CrtcPtr crtc);
void nv_crtc_hide_cursor(xf86CrtcPtr crtc);
void nv_crtc_set_cursor_position(xf86CrtcPtr crtc, int x, int y);
void nv_crtc_set_cursor_colors(xf86CrtcPtr crtc, int bg, int fg);
void nv_crtc_load_cursor_image(xf86CrtcPtr crtc, CARD8 *image);
void nv_crtc_load_cursor_argb(xf86CrtcPtr crtc, CARD32 *image);

/* in nv_dma.c */
void  NVSync(ScrnInfoPtr pScrn);
void  NVResetGraphics(ScrnInfoPtr pScrn);
Bool  NVInitDma(ScrnInfoPtr pScrn);

/* in nv_exa.c */
Bool NVExaInit(ScreenPtr pScreen);

/* in nv_hw.c */
void NVCalcStateExt(NVPtr,struct _riva_hw_state *,int,int,int,int,int,int);
void NVLoadStateExt(ScrnInfoPtr pScrn,struct _riva_hw_state *);
void NVUnloadStateExt(NVPtr,struct _riva_hw_state *);
void NVSetStartAddress(NVPtr,CARD32);
int  NVShowHideCursor(NVPtr,int);
void NVLockUnlock(NVPtr,int);
uint8_t nvReadVGA(NVPtr pNv, uint8_t index);
void nvWriteVGA(NVPtr pNv, uint8_t index, uint8_t data);
uint8_t NVReadVGA0(NVPtr pNv, uint8_t index);
uint8_t NVReadVGA1(NVPtr pNv, uint8_t index);
void NVWriteVGA0(NVPtr pNv, uint8_t index, uint8_t data);
void NVWriteVGA1(NVPtr pNv, uint8_t index, uint8_t data);
void nvWriteRAMDAC(NVPtr pNv, uint8_t head, uint32_t ramdac_reg, CARD32 val);
CARD32 nvReadRAMDAC(NVPtr pNv, uint8_t head, uint32_t ramdac_reg);
void nvWriteCRTC(NVPtr pNv, uint8_t head, uint32_t reg, CARD32 val);
CARD32 nvReadCRTC(NVPtr pNv, uint8_t head, uint32_t reg);

/* in nv_shadow.c */
void NVRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void NVRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void NVRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void NVRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void NVPointerMoved(int index, int x, int y);

/* in nv_bios.c */
unsigned int NVParseBios(ScrnInfoPtr pScrn);
void link_head_and_output(ScrnInfoPtr pScrn, int head, int dcb_entry, Bool overrideval);
void call_lvds_script(ScrnInfoPtr pScrn, int head, int dcb_entry, enum LVDS_script script, int pxclk);
void run_tmds_table(ScrnInfoPtr pScrn, int dcb_entry, int head, int pxclk);
Bool get_pll_limits(ScrnInfoPtr pScrn, enum pll_types plltype, struct pll_lims *pll_lim);
uint32_t getMNP_single(ScrnInfoPtr pScrn, uint32_t clk, int *NM, int *log2P);
uint32_t getMNP_double(ScrnInfoPtr pScrn, struct pll_lims *pll_lim, uint32_t clk, int *NM1, int *NM2, int *log2P);

void nForceUpdateArbitrationSettings (unsigned      VClk,  unsigned      pixelDepth,
				      unsigned     *burst, unsigned     *lwm,
				      NVPtr        pNv);

/* nv_crtc.c */
DisplayModePtr NVCrtcFindClosestMode(xf86CrtcPtr crtc, DisplayModePtr pMode);
void NVCrtcSetBase (xf86CrtcPtr crtc, int x, int y, Bool bios_restore);
void NVCrtcLoadPalette(xf86CrtcPtr crtc);
void NVCrtcBlankScreen(xf86CrtcPtr crtc, Bool on);
void NVCrtcSetCursor(xf86CrtcPtr crtc, Bool state);
void nv_crtc_init(ScrnInfoPtr pScrn, int crtc_num);
void NVCrtcLockUnlock(xf86CrtcPtr crtc, Bool Lock);
void NVWriteVgaCrtc(xf86CrtcPtr crtc, uint8_t index, uint8_t value);
uint8_t NVReadVgaCrtc(xf86CrtcPtr crtc, uint8_t index);
void NVWriteVGA(NVPtr pNv, int head, uint8_t index, uint8_t value);
uint8_t NVReadVGA(NVPtr pNv, int head, uint8_t index);
xf86OutputPtr NVGetOutputFromCRTC(xf86CrtcPtr crtc);
xf86CrtcPtr nv_find_crtc_by_index(ScrnInfoPtr pScrn, int index);
void NVWriteVGACR5758(NVPtr pNv, int head, uint8_t index, uint8_t value);
uint8_t NVReadVGACR5758(NVPtr pNv, int head, uint8_t index);
void NVCrtcModeFix(xf86CrtcPtr crtc);

/* nv_output.c */
void NvSetupOutputs(ScrnInfoPtr pScrn);
void NVOutputWriteRAMDAC(xf86OutputPtr output, uint32_t ramdac_reg, uint32_t val);
uint32_t NVOutputReadRAMDAC(xf86OutputPtr output, uint32_t ramdac_reg);
void NVWriteTMDS(NVPtr pNv, int ramdac, uint32_t tmds_reg, uint32_t val);
uint8_t NVReadTMDS(NVPtr pNv, int ramdac, uint32_t tmds_reg);
uint32_t nv_get_clock_from_crtc(ScrnInfoPtr pScrn, uint8_t crtc);
uint32_t nv_calc_tmds_clock_from_pll(xf86OutputPtr output);
void nv_set_tmds_registers(xf86OutputPtr output, uint32_t clock, Bool override, Bool crosswired);

/* nv_hw.c */
void nForceUpdateArbitrationSettings (unsigned VClk, unsigned pixelDepth,
				      unsigned     *burst, unsigned     *lwm,
				      NVPtr        pNv);
void nv30UpdateArbitrationSettings (NVPtr        pNv,
				    unsigned     *burst,
				    unsigned     *lwm);
void nv10UpdateArbitrationSettings (unsigned      VClk, 
				    unsigned      pixelDepth, 
				    unsigned     *burst,
				    unsigned     *lwm,
				    NVPtr        pNv);
void nv4UpdateArbitrationSettings (unsigned      VClk, 
				   unsigned      pixelDepth, 
				   unsigned     *burst,
				   unsigned     *lwm,
				   NVPtr        pNv);

void NVInitSurface(ScrnInfoPtr pScrn, RIVA_HW_STATE *state);
void NVInitGraphContext(ScrnInfoPtr pScrn);

/* in nv_i2c.c */
Bool NV_I2CInit(ScrnInfoPtr pScrn, I2CBusPtr *bus_ptr, int i2c_reg, char *name);

/* in nv04_video_overlay.c */
void NV04PutOverlayImage(ScrnInfoPtr, int, int, int, BoxPtr,
		int, int, int, int, short, short, short, short,
		short, short, RegionPtr clipBoxes);
int NV04SetOverlayPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
int NV04GetOverlayPortAttribute(ScrnInfoPtr, Atom, INT32 *, pointer);
void NV04StopOverlay(ScrnInfoPtr);

/* in nv04_video_blitter.c */
void NVPutBlitImage(ScrnInfoPtr, int, int, int, BoxPtr,
		int, int, int, int,
		short, short, short,
		short, short, short,
		RegionPtr, DrawablePtr);
int NVSetBlitPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
int NVGetBlitPortAttribute(ScrnInfoPtr, Atom, INT32 *, pointer);
void NVStopBlitVideo(ScrnInfoPtr, pointer, Bool);

/* in nv10_exa.c */
Bool NVAccelInitNV10TCL(ScrnInfoPtr pScrn);
Bool NV10CheckComposite(int, PicturePtr, PicturePtr, PicturePtr);
Bool NV10PrepareComposite(int, PicturePtr, PicturePtr, PicturePtr,
				  PixmapPtr, PixmapPtr, PixmapPtr);
void NV10Composite(PixmapPtr, int, int, int, int, int, int, int, int);
void NV10DoneComposite(PixmapPtr);

/* in nv10_video_overlay.c */
void NV10PutOverlayImage(ScrnInfoPtr, int, int, int, int, BoxPtr,
		int, int, int, int, short, short, short, short, 
		short, short, RegionPtr clipBoxes);
int NV10SetOverlayPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
int NV10GetOverlayPortAttribute(ScrnInfoPtr, Atom, INT32 *, pointer);
void NV10StopOverlay(ScrnInfoPtr);
void NV10WriteOverlayParameters(ScrnInfoPtr);

/* in nv30_exa.c */
Bool NVAccelInitNV30TCL(ScrnInfoPtr pScrn);
Bool NV30EXACheckComposite(int, PicturePtr, PicturePtr, PicturePtr);
Bool NV30EXAPrepareComposite(int, PicturePtr, PicturePtr, PicturePtr,
				  PixmapPtr, PixmapPtr, PixmapPtr);
void NV30EXAComposite(PixmapPtr, int, int, int, int, int, int, int, int);
void NV30EXADoneComposite(PixmapPtr);

/* in nv40_exa.c */
Bool NVAccelInitNV40TCL(ScrnInfoPtr pScrn);
Bool NV40EXACheckComposite(int, PicturePtr, PicturePtr, PicturePtr);
Bool NV40EXAPrepareComposite(int, PicturePtr, PicturePtr, PicturePtr,
				  PixmapPtr, PixmapPtr, PixmapPtr);
void NV40EXAComposite(PixmapPtr, int, int, int, int, int, int, int, int);
void NV40EXADoneComposite(PixmapPtr);

/* in nv40_video_texture.c */
int NV40PutTextureImage(ScrnInfoPtr, int, int, int, int, BoxPtr,
		int, int, int, int, uint16_t, uint16_t,
		uint16_t, uint16_t, uint16_t, uint16_t,
		RegionPtr, DrawablePtr);
void NV40StopTexturedVideo(ScrnInfoPtr, pointer, Bool);
int NVGetTexturePortAttribute(ScrnInfoPtr, Atom, INT32 *, pointer);
int NVSetTexturePortAttribute(ScrnInfoPtr, Atom, INT32, pointer);

/* in nv50_exa.c */
Bool NV50EXAPrepareSolid(PixmapPtr, int, Pixel, Pixel);
void NV50EXASolid(PixmapPtr, int, int, int, int);
void NV50EXADoneSolid(PixmapPtr);
Bool NV50EXAPrepareCopy(PixmapPtr, PixmapPtr, int, int, int, Pixel);
void NV50EXACopy(PixmapPtr, int, int, int, int, int, int);
void NV50EXADoneCopy(PixmapPtr);
Bool NV50EXACheckComposite(int, PicturePtr, PicturePtr, PicturePtr);
Bool NV50EXAPrepareComposite(int, PicturePtr, PicturePtr, PicturePtr,
				  PixmapPtr, PixmapPtr, PixmapPtr);
void NV50EXAComposite(PixmapPtr, int, int, int, int, int, int, int, int);
void NV50EXADoneComposite(PixmapPtr);
Bool NV50EXAUploadSIFC(ScrnInfoPtr pScrn, const char *src, int src_pitch,
		       PixmapPtr pdPix, int x, int y, int w, int h, int cpp);

/* in nv50_display.c */
Bool NV50CrtcModeFixup(xf86CrtcPtr crtc,
		DisplayModePtr mode, DisplayModePtr adjusted_mode);
void NV50CrtcPrepare(xf86CrtcPtr crtc);
void NV50CrtcModeSet(xf86CrtcPtr crtc, DisplayModePtr mode,
		DisplayModePtr adjusted_mode, int x, int y);
void NV50CrtcCommit(xf86CrtcPtr crtc);
void NV50CrtcSetPClk(xf86CrtcPtr crtc);

/* in nv50_cursor.c */
void NV50SetCursorPosition(xf86CrtcPtr crtc, int x, int y);
void NV50CrtcShowCursor(xf86CrtcPtr crtc);
void NV50CrtcHideCursor(xf86CrtcPtr crtc);
void NV50LoadCursorARGB(xf86CrtcPtr crtc, CARD32 *src);

/* in nv50_crtc.c */
void NV50DispCreateCrtcs(ScrnInfoPtr pScrn);
void NV50DisplayCommand(ScrnInfoPtr pScrn, CARD32 addr, CARD32 value);
void NV50CrtcCommand(xf86CrtcPtr crtc, CARD32 addr, CARD32 value);
void NV50CrtcWrite(xf86CrtcPtr crtc, CARD32 addr, CARD32 value);
CARD32 NV50CrtcRead(xf86CrtcPtr crtc, CARD32 addr);
void NV50DisplayWrite(ScrnInfoPtr pScrn, CARD32 addr, CARD32 value);
CARD32 NV50DisplayRead(ScrnInfoPtr pScrn, CARD32 addr);

/* in nv50_output.c */
void NV50OrWrite(ScrnInfoPtr pScrn, int or, CARD32 addr, CARD32 value);
CARD32 NV50OrRead(ScrnInfoPtr pScrn, int or, CARD32 addr);
void NV50OutputWrite(xf86OutputPtr output, CARD32 addr, CARD32 value);
CARD32 NV50OutputRead(xf86OutputPtr output, CARD32 addr);


#endif /* __NV_PROTO_H__ */

