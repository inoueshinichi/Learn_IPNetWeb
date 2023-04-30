cmake_minimum_required(VERSION 3.14.6)

cmake_minimum_required(VERSION 3.14.6)


function(make_ip_net_web BUILD_HEADERS BUILD_SOURCES BUILD_EXE_SOURCE)

    # Target
    get_filename_component(BUILD_TARGET ${BUILD_EXE_SOURCE} NAME_WE)

    # Project
    project(${BUILD_TARGET} LANGUAGES C CXX VERSION 0.1.0)

    # Exe
    add_executable(${BUILD_TARGET} ${BUILD_EXE_SOURCE})

    # Headers & Sources
    target_sources(${BUILD_TARGET} PRIVATE ${BUILD_HEADERS} PRIVATE ${BUILD_SOURCES})

    # Include Directory (Insurance)
    target_include_directories(${BUILD_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${BUILD_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}) # Top level layer

    # Library postfix
    set(CMAKE_RELEASE_POSTFIX "")
    set(CMAKE_DEBUG_POSTFIX d)
    set(CMAKE_MINSIZEREL_POSTFIX s)
    set(CMAKE_RELWITHDEBINFO_POSTFIX rd)

    # Policy
    if(POLICY CMP0025)
        cmake_policy(SET CMP0025 NEW) # AppleClang
    endif()

    if(POLICY CMP0115)
        cmake_policy(SET CMP0115 NEW) # explicit source symbol
    endif()

    ################# 
    # Configuration #
    #################
    # Definitions
    target_compile_definitions(${BUILD_TARGET} PRIVATE
        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>:WIN32 _WINDOWS _CRT_NONSTDC_NO_DEPRECATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:NDEBUG>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:_DEBUG>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:MinSizeRel>>:NDEBUG>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebgInfo>>:_DEBUG>

        # GNU, Clang, AppleClang
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:NDEBUG>
    )

    # Features
    target_compile_features(${BUILD_TARGET} PRIVATE 
        cxx_std_17 # C++17
    )

    # Options
    target_compile_options(${BUILD_TARGET} PRIVATE
        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>: /W4 /GR /EHsc /utf-8 /Zc:__cplusplus /Zc:preprocessor /bigobj> # /WXは警告をエラーと見なす
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Ob2 /O2>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/Ob0 /Od /Zi /RTC1>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:MinSizeRel>>:/O1>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebgInfo>>:/Od /Zi /RTC1>

        # GNU, Clang, AppleClang
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<CONFIG:Release>>:-O3>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<CONFIG:Debug>>:-O0 -g>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<CONFIG:MinSizeRel>>:-Os>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>,$<CONFIG:RelWithDebgInfo>>:-O2 -g>
    )

    # Options for Suppression of Warning
    target_compile_options(${BUILD_TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: /wd"4100" /wd"5054" /wd"4819" /wd"4099">
        $<$<CXX_COMPILER_ID:GNU>: >
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>: >
    )

    # Output Preprocessor(*.ii) & Assembler(*.s)
    if(ON)
        # message(STATUS "Output preprocessed files....")
        target_compile_options(${BUILD_TARGET} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/FA>
            $<$<CXX_COMPILER_ID:GNU>:-save-temps=obj>
            $<$<CXX_COMPILER_ID:Clang>:-save-temps=obj>
            $<$<CXX_COMPILER_ID:AppleClang>:-save-temps=obj>
        )
    endif()

    # OpenMP
    if(ON)
        if(NOT APPLE)
            find_package(OpenMP REQUIRED)
            target_compile_options(${BUILD_TARGET} PRIVATE
                $<$<CXX_COMPILER_ID:MSVC>:/openmp>
                $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fopenmp>
            )
        else()
            # macOSのOpenMPはGeneratorExpressionに対応していないバグがある.
            # macOSのAppleClangの場合CMakeのfildpakageスクリプトの動作にバグがありそうなので, Homebrewでインストールして直接指定.
            # https://zv-louis.hatenablog.com/entry/2018/12/23/141327
            execute_process(COMMAND brew --prefix libomp OUTPUT_VARIABLE OPENMP_HOME OUTPUT_STRIP_TRAILING_WHITESPACE)
            if(${OPENMP_HOME} STREQUAL "")
                message(FATAL_ERROR "Not found OpenMP. Please `brew install libomp`")
            endif()
            # message(STATUS "OpenMP root path: ${OPENMP_HOME}") # 非表示
            set(OpenMP_INCLUDE_DIRS "${OpenMP_HOME}/include/")
            set(OpenMP_LIBRARY "${OpenMP_HOME}/lib/")
            set(OpenMP_CXX_LIB_NAMES "libomp")
            target_compile_options(${BUILD_TARGET} PRIVATE
                $<$<CXX_COMPILER_ID:AppleClang>:-Xpreprocessor -fopenmp> # AppleClang with XCode
            )
        endif()
        target_include_directories(${BUILD_TARGET} PRIVATE ${OpenMP_INCLUDE_DIRS})
        target_link_directories(${BUILD_TARGET} PRIVATE ${OpenMP_LIBRARY})
        target_link_libraries(${BUILD_TARGET} PRIVATE ${OpenMP_LIBRARIES})
    endif()

    ##############
    # Properties #
    ##############
    if(UNIX OR APPLE)
        # Makefileの詳細情報を出力
        set(CMAKE_VERBOSE_MAKEFILE ON)
    endif()

    if(MSVC)
        if(OFF)
            set_target_properties(${BUILD_TARGET} PROPERTIES
                MSVC_RUNTIME_LIBRARY MultiThreaded$<CONFIG:Release>>:> # -MT (MultiThreaded)
                MSVC_RUNTIME_LIBRARY MultiThreaded$<CONFIG:Debug>>:Debug> # -MTd (MultiThreadedDebug)
            )
        else()
            set_target_properties(${BUILD_TARGET} PROPERTIES
                    MSVC_RUNTIME_LIBRARY MultiThreaded$<CONFIG:Release>>:DLL> # -MD (MultiThreadedDLL)
                    MSVC_RUNTIME_LIBRARY MultiThreaded$<CONFIG:Debug>>:DebugDLL> # -MDd (MultiThreadedDebugDLL)
            )
        endif()
    endif()

    #################
    # Debug utility #
    #################
    # message(STATUS "Output preprocessed files....")
    target_compile_options(${BUILD_TARGET} PRIVATE
        # $<$<CXX_COMPILER_ID:MSVC>:>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wshadow> # シャドウイングの警告
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wconversion> # 暗黙の型変換の警告
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wfloat-equal> # 実数値の等号比較を警告
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-ftrapv> # オーバーフロー検出(検出すると即座にプログラム停止)
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fstack-protector-all> # スタック領域の範囲外書き込みの検出
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fsanitize=address> # 不正メモリ操作の検出
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fsanitize=undefined> # 未定義動作の検出
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fno-omit-frame-pointer> # 有効なスタックトレースの抽出
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-ggdb> # GDB向けのデバッグ情報を埋め込む
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-fverbose-asm> # アセンブリコードにソースコード情報を埋め込む
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:>
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:>
        # $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:>
    )

endfunction()