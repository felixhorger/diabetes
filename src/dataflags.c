
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
#define EVALINFOMASK_ACQEND			1l << 0  // Last readout in scan
#define EVALINFOMASK_RTFEEDBACK			1l << 1  // Realtime feedback
#define EVALINFOMASK_HPFEEDBACK			1l << 2  // High perfomance feedback
#define EVALINFOMASK_ONLINE			1l << 3  // Online processing for recon?
#define EVALINFOMASK_OFFLINE			1l << 4  // Offline processing (question: doesn't !offline -> online?)
#define EVALINFOMASK_SYNCDATA			1l << 5  // Readout contains synchroneous data (synchroneous?)
#define EVALINFOMASK_UNNAMED6			1l << 6
#define EVALINFOMASK_UNNAMED7			1l << 7
#define EVALINFOMASK_LASTSCANINCONCAT		1l << 8  // Last scan in concatenation (I love the seqloop)
#define EVALINFOMASK_UNNAMED9			1l << 9
#define EVALINFOMASK_RAWDATACORRECTION		1l << 10 // Use raw data correction factor
#define EVALINFOMASK_LASTSCANINMEAS		1l << 11 // Last scan in measurement (nice ordering of flags, well done)
#define EVALINFOMASK_SCANSCALEFACTOR		1l << 12 // Use scan specific additional scale (I guess similar to raw data correction factor)
#define EVALINFOMASK_SECONDHADAMARPULSE		1l << 13 // Second RF exitation of Hadamar pulse
#define EVALINFOMASK_REFPHASESTABSCAN		1l << 14 // Reference phase stabilization scan
#define EVALINFOMASK_PHASESTABSCAN		1l << 15 // Phase stabilization scan
#define EVALINFOMASK_D3FFT			1l << 16 // Execute 3D FFT
#define EVALINFOMASK_SIGNREV			1l << 17 // Sign reversal
#define EVALINFOMASK_PHASEFFT			1l << 18 // Execute phase fft
#define EVALINFOMASK_SWAPPED			1l << 19 // Swapped phase and readout direction
#define EVALINFOMASK_POSTSHAREDLINE		1l << 20 // Shared line
#define EVALINFOMASK_PHASCOR			1l << 21 // Phase correction data
#define EVALINFOMASK_PATREFSCAN			1l << 22 // Additional scan for partial Fourier reference line/partition
#define EVALINFOMASK_PATREFANDIMASCAN		1l << 23 // Partial Fourier reference that is also used as image scan
#define EVALINFOMASK_REFLECT			1l << 24 // Reflect line (along readout or what?)
#define EVALINFOMASK_NOISEADJSCAN		1l << 25 // Noise adjust scan
#define EVALINFOMASK_SHARENOW			1l << 26 // Lines (readouts?) may be shared between e.g. phases
#define EVALINFOMASK_LASTMEASUREDLINE		1l << 27 // Last phase (lines) which is measured, I guess
#define EVALINFOMASK_FIRSTSCANINSLICE		1l << 28 // First scan in slice
#define EVALINFOMASK_LASTSCANINSLICE		1l << 29 // Last scan in slice
#define EVALINFOMASK_TREFFECTIVEBEGIN		1l << 30 // Effective TR begin (triggered from realtime feedback?)
#define EVALINFOMASK_TREFFECTIVEEND		1l << 31 // Effective TR end (also triggered)
#define EVALINFOMASK_REF_POSITION		1l << 32 // Reference position for (patient?) movement
#define EVALINFOMASK_SLC_AVERAGED		1l << 33 // Averaged slice
#define EVALINFOMASK_TAGFLAG1			1l << 34 // Adjust scans
#define EVALINFOMASK_CT_NORMALIZE		1l << 35 // Correction scan for TimCT prescan normalise
#define EVALINFOMASK_SCAN_FIRST			1l << 36 // First scan of a particular map
#define EVALINFOMASK_SCAN_LAST			1l << 37 // Last scan of a particular map
#define EVALINFOMASK_SLICE_ACCEL_REFSCAN        1l << 38 // Single-band reference scan for multi-band
#define EVALINFOMASK_SLICE_ACCEL_PHASCOR	1l << 39 // Additional phase correction in multi-band
#define EVALINFOMASK_FIRST_SCAN_IN_BLADE	1l << 40 // The first line of a blade
#define EVALINFOMASK_LAST_SCAN_IN_BLADE		1l << 41 // The last line of a blade
#define EVALINFOMASK_LAST_BLADE_IN_TR		1l << 42 // Is true for all lines of last BLADE in each TR
#define EVALINFOMASK_UNNAMED43			1l << 43
#define EVALINFOMASK_PACE			1l << 44 // PACE scan
#define EVALINFOMASK_RETRO_LASTPHASE		1l << 45 // Last phase in a heartbeat
#define EVALINFOMASK_RETRO_ENDOFMEAS		1l << 46 // Readout is at end of measurement
#define EVALINFOMASK_RETRO_REPEATTHISHEARTBEAT	1l << 47 // Repeat the current heartbeat
#define EVALINFOMASK_RETRO_REPEATPREVHEARTBEAT	1l << 48 // Repeat the previous heartbeat
#define EVALINFOMASK_RETRO_ABORTSCANNOW		1l << 49 // Abort everything
#define EVALINFOMASK_RETRO_LASTHEARTBEAT	1l << 50 // Readout is from last heartbeat (a dummy)
#define EVALINFOMASK_RETRO_DUMMYSCAN		1l << 51 // Readout is just a dummy scan, throw it away
#define EVALINFOMASK_RETRO_ARRDETDISABLED	1l << 52 // Disable all arrhythmia detection
#define EVALINFOMASK_B1_CONTROLLOOP		1l << 53 // Use readout for B1 Control Loop
#define EVALINFOMASK_SKIP_ONLINE_PHASCOR	1l << 54 // Do not correct phases online
#define EVALINFOMASK_SKIP_REGRIDDING		1l << 55 // Do not regrid
#define EVALINFOMASK_MDH_VOP			1l << 56 // VOP based RF monitoring
#define EVALINFOMASK_UNNAMED57			1l << 57
#define EVALINFOMASK_UNNAMED58			1l << 58
#define EVALINFOMASK_UNNAMED59			1l << 59
#define EVALINFOMASK_UNNAMED60			1l << 60
#define EVALINFOMASK_WIP_1			1l << 61 // WIP application type 1
#define EVALINFOMASK_WIP_2			1l << 62 // WIP application type 2
#define EVALINFOMASK_WIP_3			1l << 63 // WIP application type 3


#define NO_IMAGE_SCAN                 ( \
	EVALINFOMASK_ACQEND           | \
	EVALINFOMASK_RTFEEDBACK       | \
	EVALINFOMASK_HPFEEDBACK       | \
	EVALINFOMASK_SYNCDATA         | \
	EVALINFOMASK_REFPHASESTABSCAN | \
	EVALINFOMASK_PHASESTABSCAN    | \
	EVALINFOMASK_PHASCOR          | \
	EVALINFOMASK_NOISEADJSCAN     | \
	EVALINFOMASK_UNNAMED60          \
)

