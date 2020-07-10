set(SOURCES
    MatroskaDocument.h
    MatroskaReader.cpp
    VideoPlayerWidget.cpp
    VP9Decoder.cpp
)

serenity_lib(LibVideo video)
target_link_libraries(LibVideo LibAudio LibCore LibIPC)
