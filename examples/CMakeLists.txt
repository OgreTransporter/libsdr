IF(SDR_WITH_PORTAUDIO)
 add_executable(sdr_wavplay sdr_wavplay.cc)
 target_link_libraries(sdr_wavplay ${LIBS} libsdr)

 add_executable(sdr_fm sdr_fm.cc)
 target_link_libraries(sdr_fm ${LIBS} libsdr)

 add_executable(sdr_rec sdr_rec.cc)
 target_link_libraries(sdr_rec ${LIBS} libsdr)

 add_executable(sdr_rtty sdr_rtty.cc)
 target_link_libraries(sdr_rtty ${LIBS} libsdr)

 add_executable(sdr_pocsag sdr_pocsag.cc)
 target_link_libraries(sdr_pocsag ${LIBS} libsdr)

 add_executable(sdr_ax25 sdr_ax25.cc)
 target_link_libraries(sdr_ax25 ${LIBS} libsdr)

ENDIF(SDR_WITH_PORTAUDIO)


