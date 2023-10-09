
/*
	It is quite striking that it is very specific to Cartesian imaging.
	I assume this has historic reasons, and now it is what it is, fair enough.
	However, it is outrageous that the description of scans is so strongly entangled with
	a standard format for raw data. To give a couple of examples, there are flags related to heart beats,
	BLADE sampling, or partial Fourier.
	Since it is difficult to define a fixed standard format that includes all relevant information
	for current and future types of scans, the separation of raw-data and information about the
	applied technique follows logically.
*/
#define EVALINFOMASK_ACQEND			1 << 0  // Last readout in scan
#define EVALINFOMASK_RTFEEDBACK			1 << 1  // Realtime feedback
#define EVALINFOMASK_HPFEEDBACK			1 << 2  // High perfomance feedback
#define EVALINFOMASK_ONLINE			1 << 3  // Online processing for recon?
#define EVALINFOMASK_OFFLINE			1 << 4  // Offline processing (question: doesn't !offline -> online?)
#define EVALINFOMASK_SYNCDATA			1 << 5  // Readout contains synchroneous data (synchroneous?)
#define EVALINFOMASK_UNNAMED6			1 << 6
#define EVALINFOMASK_UNNAMED7			1 << 7
#define EVALINFOMASK_LASTSCANINCONCAT		1 << 8  // Last scan in concatenation (I love the seqloop)
#define EVALINFOMASK_UNNAMED9			1 << 9
#define EVALINFOMASK_RAWDATACORRECTION		1 << 10 // Use raw data correction factor
#define EVALINFOMASK_LASTSCANINMEAS		1 << 11 // Last scan in measurement (nice ordering of flags, well done)
#define EVALINFOMASK_SCANSCALEFACTOR		1 << 12 // Use scan specific additional scale (I guess similar to raw data correction factor)
#define EVALINFOMASK_SECONDHADAMARPULSE		1 << 13 // Second RF exitation of Hadamar pulse
#define EVALINFOMASK_REFPHASESTABSCAN		1 << 14 // Reference phase stabilization scan
#define EVALINFOMASK_PHASESTABSCAN		1 << 15 // Phase stabilization scan
#define EVALINFOMASK_D3FFT			1 << 16 // Execute 3D FFT
#define EVALINFOMASK_SIGNREV			1 << 17 // Sign reversal
#define EVALINFOMASK_PHASEFFT			1 << 18 // Execute phase fft
#define EVALINFOMASK_SWAPPED			1 << 19 // Swapped phase and readout direction
#define EVALINFOMASK_POSTSHAREDLINE		1 << 20 // Shared line
#define EVALINFOMASK_PHASCOR			1 << 21 // Phase correction data
#define EVALINFOMASK_PATREFSCAN			1 << 22 // Additional scan for partial Fourier reference line/partition
#define EVALINFOMASK_PATREFANDIMASCAN		1 << 23 // Partial Fourier reference that is also used as image scan
#define EVALINFOMASK_REFLECT			1 << 24 // Reflect line (along readout or what?)
#define EVALINFOMASK_NOISEADJSCAN		1 << 25 // Noise adjust scan
#define EVALINFOMASK_SHARENOW			1 << 26 // Lines (readouts?) may be shared between e.g. phases
#define EVALINFOMASK_LASTMEASUREDLINE		1 << 27 // Last phase (lines) which is measured, I guess
#define EVALINFOMASK_FIRSTSCANINSLICE		1 << 28 // First scan in slice
#define EVALINFOMASK_LASTSCANINSLICE		1 << 29 // Last scan in slice
#define EVALINFOMASK_TREFFECTIVEBEGIN		1 << 30 // Effective TR begin (triggered from realtime feedback?)
#define EVALINFOMASK_TREFFECTIVEEND		1 << 31 // Effective TR end (also triggered)
#define EVALINFOMASK_REF_POSITION		1 << 32 // Reference position for (patient?) movement
#define EVALINFOMASK_SLC_AVERAGED		1 << 33 // Averaged slice
#define EVALINFOMASK_TAGFLAG1			1 << 34 // Adjust scans
#define EVALINFOMASK_CT_NORMALIZE		1 << 35 // Correction scan for TimCT prescan normalise
#define EVALINFOMASK_SCAN_FIRST			1 << 36 // First scan of a particular map
#define EVALINFOMASK_SCAN_LAST			1 << 37 // Last scan of a particular map
#define EVALINFOMASK_SLICE_ACCEL_REFSCAN        1 << 38 // Single-band reference scan for multi-band
#define EVALINFOMASK_SLICE_ACCEL_PHASCOR	1 << 39 // Additional phase correction in multi-band
#define EVALINFOMASK_FIRST_SCAN_IN_BLADE	1 << 40 // The first line of a blade
#define EVALINFOMASK_LAST_SCAN_IN_BLADE		1 << 41 // The last line of a blade
#define EVALINFOMASK_LAST_BLADE_IN_TR		1 << 42 // Is true for all lines of last BLADE in each TR
#define EVALINFOMASK_UNNAMED43			1 << 43
#define EVALINFOMASK_PACE			1 << 44 // PACE scan
#define EVALINFOMASK_RETRO_LASTPHASE		1 << 45 // Last phase in a heartbeat
#define EVALINFOMASK_RETRO_ENDOFMEAS		1 << 46 // Readout is at end of measurement
#define EVALINFOMASK_RETRO_REPEATTHISHEARTBEAT	1 << 47 // Repeat the current heartbeat
#define EVALINFOMASK_RETRO_REPEATPREVHEARTBEAT	1 << 48 // Repeat the previous heartbeat
#define EVALINFOMASK_RETRO_ABORTSCANNOW		1 << 49 // Abort everything
#define EVALINFOMASK_RETRO_LASTHEARTBEAT	1 << 50 // Readout is from last heartbeat (a dummy)
#define EVALINFOMASK_RETRO_DUMMYSCAN		1 << 51 // Readout is just a dummy scan, throw it away
#define EVALINFOMASK_RETRO_ARRDETDISABLED	1 << 52 // Disable all arrhythmia detection
#define EVALINFOMASK_B1_CONTROLLOOP		1 << 53 // Use readout for B1 Control Loop
#define EVALINFOMASK_SKIP_ONLINE_PHASCOR	1 << 54 // Do not correct phases online
#define EVALINFOMASK_SKIP_REGRIDDING		1 << 55 // Do not regrid
#define EVALINFOMASK_MDH_VOP			1 << 56 // VOP based RF monitoring
#define EVALINFOMASK_UNNAMED57			1 << 57
#define EVALINFOMASK_UNNAMED58			1 << 58
#define EVALINFOMASK_UNNAMED59			1 << 59
#define EVALINFOMASK_UNNAMED60			1 << 60
#define EVALINFOMASK_WIP_1			1 << 61 // WIP application type 1
#define EVALINFOMASK_WIP_2			1 << 62 // WIP application type 2
#define EVALINFOMASK_WIP_3			1 << 63 // WIP application type 3

