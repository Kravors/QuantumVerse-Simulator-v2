cmake_minimum_required(VERSION 3.16)

# QuantumVerse Simulator - CMake Build System
# This CMakeLists.txt currently configures the new dilaton gravity module.
# It will be extended to include the full project (spacetime, physics, UI, etc.)
# in subsequent sprints.

project(QuantumVerse LANGUAGES C CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Sanitizer Support (for memory leak and undefined behavior detection)
# =============================================================================
option(QUANTUMVERSE_USE_ASAN "Enable AddressSanitizer for memory leak detection" OFF)
option(QUANTUMVERSE_USE_TSAN "Enable ThreadSanitizer for race detection" OFF)
option(QUANTUMVERSE_USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(QUANTUMVERSE_ASAN_STATIC "Statically link ASan runtime (avoids DLL on Windows)" OFF)

if(QUANTUMVERSE_USE_ASAN AND MSVC)
    # MSVC uses /fsanitize=address (Visual Studio 2019 16.10+)
    # Note: MSVC's ASan requires the Visual Studio runtime
    # For Windows, recommend using Clang with -static or MSVC without ASan
    if(QUANTUMVERSE_ASAN_STATIC)
        # Try to use static runtime - this may still have issues with ASan on Windows
        # Best solution: use Clang compiler instead of MSVC for ASan on Windows
        message(WARNING "MSVC static ASan may have issues on Windows. Consider using Clang compiler with -static-libgcc.")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    message(STATUS "AddressSanitizer enabled for MSVC")
    add_compile_options(/fsanitize=address)
    add_link_options(/INCREMENTAL:NO)
elseif(QUANTUMVERSE_USE_ASAN)
    # GCC/Clang AddressSanitizer
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    if(QUANTUMVERSE_ASAN_STATIC)
        # Static linking avoids the need for clang_rt.asan_dynamic-x86_64.dll on Windows
        if(WIN32)
            add_link_options(-static)
        endif()
        message(STATUS "ASan static linking enabled - no DLL required")
    endif()
endif()

if(QUANTUMVERSE_USE_UBSAN AND NOT MSVC)
    # UBSan only available on GCC/Clang
    message(STATUS "UndefinedBehaviorSanitizer enabled")
    add_compile_options(-fsanitize=undefined -fno-sanitize-recover=undefined)
    add_link_options(-fsanitize=undefined)
endif()

if(QUANTUMVERSE_USE_TSAN AND NOT MSVC)
    # TSan only available on GCC/Clang
    message(STATUS "ThreadSanitizer enabled")
    add_compile_options(-fsanitize=thread -g)
    add_link_options(-fsanitize=thread)
endif()

# =============================================================================
# Build Options
# =============================================================================
option(QUANTUMVERSE_BUILD_TESTS "Build test executables" ON)
option(QUANTUMVERSE_FORMAT_CODE "Format source code with clang-format" OFF)

# =============================================================================
# Advanced Error Discovery Options
# =============================================================================
option(QUANTUMVERSE_ENABLE_CBMC "Enable CBMC formal verification" OFF)
option(QUANTUMVERSE_ENABLE_KLEE "Enable KLEE symbolic execution" OFF)
option(QUANTUMVERSE_USE_BOOST_MULTIPRECISION "Build formal verification tests with Boost multiprecision" OFF)
option(QUANTUMVERSE_USE_LONG_DOUBLE "Build with long double precision for differential testing" OFF)
option(QUANTUMVERSE_USE_GOLDEN_MASTER "Enable golden master image comparison tests" OFF)

# =============================================================================
# Clang-Format Integration
# =============================================================================
find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
    
    # Collect all source files
    file(GLOB_RECURSE ALL_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp"
    )
    
    if(QUANTUMVERSE_FORMAT_CODE)
        message(STATUS "Formatting ${CMAKE_CURRENT_SOURCE_DIR}/src and tests")
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/.formatted
            COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_SOURCES}
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/.formatted
            COMMENT "Formatting source files with clang-format"
            VERBATIM
        )
        add_custom_target(format ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/.formatted)
    endif()
    
    # Add a target to check formatting without modifying
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -style=file ${ALL_SOURCES}
        COMMENT "Checking code formatting"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found - code formatting target not available")
endif()

# =============================================================================
# Clang-Tidy Custom Checks for Physics Invariants
# =============================================================================
option(QUANTUMVERSE_ENFORCE_TIDY_ERRORS "Treat clang-tidy warnings as errors" ON)
set(QUANTUMVERSE_PHYSICS_CHECKS
    "physics-invariant-metric-symmetry,
     physics-invariant-lorentz-signature,
     physics-invariant-negative-determinant,
     physics-invariant-energy-conservation"
    CACHE STRING "Custom physics invariant checks"
)

# =============================================================================
# Clang-Tidy Integration
# =============================================================================
find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*,-clang-analyzer-alpha.*;-warnings-as-errors=*")
else()
    message(STATUS "clang-tidy not found - static analysis not available")
endif()
option(QUANTUMVERSE_USE_IMGUI "Build the 4D ImGui GUI" ON)
set(QUANTUMVERSE_IMGUI_RENDERER "opengl3" CACHE STRING "ImGui renderer backend for quantumverse_imgui: dx11 or opengl3")
set_property(CACHE QUANTUMVERSE_IMGUI_RENDERER PROPERTY STRINGS dx11 opengl3)
option(QUANTUMVERSE_DX11_DEBUG_LAYER "Enable the D3D11 debug layer for the ImGui DX11 backend" OFF)
option(QUANTUMVERSE_USE_QML "Build the QML 4D GUI" OFF)
option(QUANTUMVERSE_USE_QT "Build the 4D Qt GUI (deprecated, use QUANTUMVERSE_USE_QML)" OFF)
option(QUANTUMVERSE_USE_ONNX "Build with ONNX Runtime support" OFF)

# =============================================================================
# ONNX Runtime Configuration (if enabled)
# =============================================================================
if(QUANTUMVERSE_USE_ONNX)
    # Find ONNX Runtime - try system first, then bundled
    find_package(onnxruntime CONFIG QUIET)
    
    if(onnxruntime_FOUND)
        message(STATUS "ONNX Runtime found via CMake config")
        set(ONNX_RUNTIME_LIB onnxruntime::onnxruntime)
    else()
        # Try bundled location (platform-specific)
        if(WIN32)
            set(ONNX_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/onnxruntime-win-x64-1.22.1")
        elseif(APPLE)
            set(ONNX_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/onnxruntime-osx-x64-1.22.1")
        else()
            set(ONNX_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/onnxruntime-linux-x64-1.22.1")
        endif()
        
        if(EXISTS "${ONNX_RUNTIME_ROOT}/include" AND EXISTS "${ONNX_RUNTIME_ROOT}/lib")
            message(STATUS "ONNX Runtime found at: ${ONNX_RUNTIME_ROOT}")
            
            # Add ONNX include directory
            include_directories(${ONNX_RUNTIME_ROOT}/include)
            
            # Add ONNX library directory
            link_directories(${ONNX_RUNTIME_ROOT}/lib)
            
            # Define HAVE_ONNX for conditional compilation
            add_compile_definitions(HAVE_ONNX)
            
            # Link against ONNX Runtime library
            set(ONNX_RUNTIME_LIB onnxruntime)
        else()
            message(WARNING "ONNX Runtime not found at ${ONNX_RUNTIME_ROOT} - ONNX features will be disabled")
            set(QUANTUMVERSE_USE_ONNX OFF)
        endif()
    endif()
endif()

# Ensure only one UI backend is active
if(QUANTUMVERSE_USE_QT AND QUANTUMVERSE_USE_IMGUI)
    message(WARNING "QUANTUMVERSE_USE_QT and QUANTUMVERSE_USE_IMGUI both set - using ImGui")
    set(QUANTUMVERSE_USE_QT OFF)
endif()

# =============================================================================
# GLFW Configuration (if ImGui is enabled)
# =============================================================================
if(QUANTUMVERSE_USE_IMGUI AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64")
    # Build bundled GLFW as static library
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/glfw)
endif()

# =============================================================================
# Qt6 Configuration (if QML is enabled)
# =============================================================================
if(QUANTUMVERSE_USE_QML OR QUANTUMVERSE_USE_QT)
    # Set QT_NO_NAMESPACE before find_package to prevent Qt from using the
    # project name "QuantumVerse" as a namespace (which would cause issues)
    set(QT_NAMESPACE "" CACHE STRING "Qt namespace override")
    set(QT_NO_NAMESPACE ON CACHE BOOL "Disable Qt namespace")
    
    # Find Qt6 - use system Qt or fallback to bundled version
    # Only set Qt6_DIR if not already defined by the user
    if(NOT DEFINED Qt6_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Qt6")
        # Try to find Qt6 in the workspace directory (platform-specific)
        if(WIN32)
            set(Qt6_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Qt6/6.5.3/msvc2019_64/lib/cmake/Qt6")
        elseif(APPLE)
            set(Qt6_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Qt6/6.5.3/macos/lib/cmake/Qt6")
        else()
            set(Qt6_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Qt6/6.5.3/gcc/lib/cmake/Qt6")
        endif()
    endif()
    
    find_package(Qt6 QUIET COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Qml Quick)
    
    if(Qt6_FOUND)
        message(STATUS "Qt6 found - building QML GUI")
    else()
        message(WARNING "Qt6 not found - falling back to ImGui")
        set(QUANTUMVERSE_USE_QML OFF)
        set(QUANTUMVERSE_USE_IMGUI ON)
    endif()
endif()

# =============================================================================
# ImGui Configuration (if ImGui is enabled)
# =============================================================================
if(QUANTUMVERSE_USE_IMGUI)
    message(STATUS "Building ImGui GUI")
    
    # ImGui executable
    add_executable(quantumverse_imgui
        src/main_glfw.cpp
        src/ui_imgui/UI4D_ImGui.cpp
        src/ui4d/UI4D.cpp
        src/ui4d/Camera4D.cpp
        src/ui4d/Camera4DAdapter.cpp
         src/ui4d/SceneGraph.cpp
         src/ui4d/SceneGraphManager.cpp
         src/ui4d/PlanckMicroscope.cpp
         src/ui4d/EnhancedLayout.cpp
         src/ui4d/CausalGraph.cpp
         src/ui4d/DiscoveryProbe.cpp
        src/rendering/CurvatureRenderer.cpp
        src/rendering/QuantumGeometryRenderer.cpp
        src/rendering/CelestialBodyRenderer.cpp
        src/rendering/ProceduralTextures.cpp
        src/rendering/Texture.cpp
        # Post-processing components (Phase 4)
        src/rendering/HDRFramebuffer.cpp
        src/rendering/BloomRenderer.cpp
        src/rendering/ToneMapper.cpp
        src/rendering/CinematicEffects.cpp
        src/rendering/PostProcessingPipeline.cpp
        src/quantumgravity/CDTEngine.cpp
        src/quantumgravity/CausalSet.cpp
        src/quantumgravity/SpinFoam.cpp
        src/quantumgravity/SpinNetwork.cpp
        src/quantumgravity/GFTEngine.cpp
        src/quantumgravity/SpinFoamEngine.cpp
        src/discovery/DiscoveryPanelManager.cpp
        src/discovery/DiscoveryEngine.cpp
        src/discovery/TheoryManager.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/PatchedConicIntegrator.cpp
        src/discovery/PlanetaryGridGenerator.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/RLDiscoveryAgent.cpp
        src/discovery/SymbolicMath.cpp
        src/discovery/HolographicDualityLab.cpp
        src/ml/CurvatureNormalizingFlow.cpp
        src/ml/DifferentiableSimulator.cpp
        src/ml/GeodesicNeuralODE.cpp
        src/ml/MetricGNN.cpp
        third_party/glad/glad.cpp
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_demo.cpp
        third_party/imgui/backends/imgui_impl_glfw.cpp
        # ImPlot for data visualization
        third_party/implot/implot.cpp
        third_party/implot/implot_items.cpp
        # UI ImGui components
        src/ui_imgui/AccessibilityManager.cpp
        src/ui_imgui/ContextMenu.cpp
        src/ui_imgui/FabricMeshRenderer.cpp
        src/ui_imgui/FlightTelemetryPanel.cpp
        src/ui_imgui/FloatingPanels.cpp
        src/ui_imgui/LayoutPresets.cpp
        src/ui_imgui/MinimapPanel.cpp
        src/ui_imgui/ObjectBrowser.cpp
        src/ui_imgui/ObjectPicker.cpp
        src/ui_imgui/OnboardingPanel.cpp
        src/ui_imgui/PerformanceProfiler.cpp
        src/ui_imgui/PropertyEditor.cpp
        src/ui_imgui/ScriptInterface.cpp
        src/ui_imgui/SpacetimeFlightController.cpp
        src/ui_imgui/TheorySelectorPanel.cpp
        src/ui_imgui/TimelineBar.cpp
        src/ui_imgui/ToolPalette.cpp
        src/ui_imgui/UndoRedoSystem.cpp
        src/ui_imgui/VRIntegration.cpp
        src/ui_imgui/ScreenshotCapture.cpp
        src/ui_imgui/style/UIStyleManager.cpp
        # Cosmology Panel
        src/ui_imgui/CosmologyPanel.cpp
        # Post-processing UI panel (Phase 4)
        src/ui_imgui/PostProcessingPanel.cpp
        # Cosmology module
            src/cosmology/CosmicParameters.cpp
            src/cosmology/CosmicScaleFactor.cpp
            src/cosmology/FriedmannSolver.cpp
            src/cosmology/CMBCalculator.cpp
            src/cosmology/DarkEnergyModel.cpp
        # Spacetime components
        src/spacetime/LightCone.cpp
        # Physics components
        src/physics/CurvatureCalculator.cpp
        # Utils components
         src/utils/ConfigManager.cpp
         src/utils/PerformanceMonitor.cpp
         src/utils/TraceLogger.cpp
         src/utils/CrashHandler.cpp
         src/utils/DiagnosticMode.cpp
         src/rendering/GLDebug.cpp
     )
    
    # Initialize the backend sources list
    set(imgui_backend_sources "")
    
    string(TOLOWER "${QUANTUMVERSE_IMGUI_RENDERER}" imgui_renderer_lower)
    if(imgui_renderer_lower STREQUAL "opengl3")
        list(APPEND imgui_backend_sources third_party/imgui/backends/imgui_impl_opengl3.cpp)
    else()
        if(NOT imgui_renderer_lower STREQUAL "dx11")
            message(WARNING "Unknown QUANTUMVERSE_IMGUI_RENDERER='${QUANTUMVERSE_IMGUI_RENDERER}'. Using dx11.")
        endif()
        list(APPEND imgui_backend_sources third_party/imgui/backends/imgui_impl_dx11.cpp)
        target_compile_definitions(quantumverse_imgui PRIVATE QUANTUMVERSE_IMGUI_DX11)
        if(QUANTUMVERSE_DX11_DEBUG_LAYER)
            target_compile_definitions(quantumverse_imgui PRIVATE QUANTUMVERSE_DX11_DEBUG_LAYER)
            message(STATUS "D3D11 debug layer enabled for quantumverse_imgui")
        endif()
    endif()
    target_sources(quantumverse_imgui PRIVATE ${imgui_backend_sources})
    target_compile_definitions(quantumverse_imgui PRIVATE
       QUANTUMVERSE_USE_IMGUI
    )

    # Find OpenGL
    find_package(OpenGL REQUIRED)
    
    # Find GLFW - check for pre-built or use bundled version
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64")
        # Use pre-built GLFW
        target_include_directories(quantumverse_imgui PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/include
        )
        target_link_directories(quantumverse_imgui PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/lib-vc2022
        )
        target_link_libraries(quantumverse_imgui PRIVATE glfw3)
    else()
        # Use bundled GLFW (already built via add_subdirectory above)
        target_link_libraries(quantumverse_imgui PRIVATE glfw)
    endif()
    
    # Add ImGui and ImPlot include directories (imgui must come first - ImPlot includes imgui.h)
    target_include_directories(quantumverse_imgui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot
    )
    
    target_link_libraries(quantumverse_imgui PRIVATE
        OpenGL::GL
    )
    if(NOT imgui_renderer_lower STREQUAL "opengl3")
        target_link_libraries(quantumverse_imgui PRIVATE
            d3d11
            dxgi
            d3dcompiler
        )
    endif()
    
    # Link ONNX Runtime if enabled
    if(QUANTUMVERSE_USE_ONNX AND ONNX_RUNTIME_LIB)
        target_link_libraries(quantumverse_imgui PRIVATE ${ONNX_RUNTIME_LIB})
    endif()
endif()

# Include directories: all headers are under src/
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/Catch2/include)

# ----------------------------------------------------------------------
# Dilaton Gravity Module (2D Black Hole & Hawking Radiation)
# ----------------------------------------------------------------------
add_library(dilaton
    src/spacetime/DilatonMetric.cpp
    src/physics/DilatonBlackHole.cpp
    src/physics/HawkingCalculator.cpp
    src/physics/CurvatureCalculator.cpp
    src/physics/GRRayTracer.cpp
    # Rendering components for tests
    src/rendering/CurvatureRenderer.cpp
    src/rendering/QuantumGeometryRenderer.cpp
    src/rendering/CelestialBodyRenderer.cpp
    src/rendering/ProceduralTextures.cpp
    src/rendering/Texture.cpp
    src/rendering/ValidationHarness.cpp
    # Discovery components for tests
    src/discovery/DiscoveryEngine.cpp
    src/discovery/TheoryManager.cpp
    src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
    src/discovery/PatchedConicIntegrator.cpp
    src/discovery/PlanetaryGridGenerator.cpp
    src/discovery/GalacticRotationCurveScanner.cpp
    src/discovery/FineStructureConstantDriftObservatory.cpp
    src/discovery/BosonStarCollisionPredictor.cpp
    src/discovery/NeutronStarGlitchPhaseDetector.cpp
    src/discovery/UltralightDMWaveInterferometer.cpp
    src/discovery/BlackHoleJetAnomalyRecogniser.cpp
    src/discovery/PrimordialLithiumCrisisSolver.cpp
    src/discovery/GalacticTidalStreamCartographer.cpp
    src/discovery/RecombinationConstantVariationImager.cpp
    src/discovery/RLDiscoveryAgent.cpp
    src/discovery/SymbolicMath.cpp
    src/discovery/HolographicDualityLab.cpp
    # ML components for tests
         src/ml/CurvatureNormalizingFlow.cpp
         src/ml/DifferentiableSimulator.cpp
         src/ml/GeodesicNeuralODE.cpp
         src/ml/MetricGNN.cpp
         src/ml/QuantumMetricReconstruction.cpp
         # Bayesian evidence calculator
         src/bayesian/BayesianEvidenceCalculator.cpp
         # Paper generator
         src/paper/PaperGenerator.cpp
         # VR components (stub mode - no Qt required)
         src/vr/MultiUserServer.cpp
         src/vr/VRClient.cpp
         src/vr/VRCommon.cpp
         src/vr/CollaborationHub.cpp
         # Quantum gravity components for tests
         src/quantumgravity/CDTEngine.cpp
         src/quantumgravity/CausalSet.cpp
         src/quantumgravity/SpinFoam.cpp
         src/quantumgravity/SpinNetwork.cpp
         src/quantumgravity/GFTEngine.cpp
         src/quantumgravity/SpinFoamEngine.cpp
         src/quantumgravity/ReplicaWormhole.cpp
         # Spacetime components
         src/spacetime/LightCone.cpp
         # Utils components
            src/utils/ConfigManager.cpp
            src/utils/PerformanceMonitor.cpp
            src/utils/TraceLogger.cpp
            src/utils/CrashHandler.cpp
            src/rendering/GLDebug.cpp
            src/utils/DiagnosticMode.cpp
            # Physics validation components
            src/physics/PhysicsValidator.cpp
            # UI4D components
                 src/ui4d/UI4D.cpp
                 src/ui4d/Camera4D.cpp
                 src/ui4d/Camera4DAdapter.cpp
                 src/ui4d/SceneGraph.cpp
                 src/ui4d/SceneGraphManager.cpp
                 src/ui4d/PlanckMicroscope.cpp
           src/ui4d/EnhancedLayout.cpp
           src/ui4d/CausalGraph.cpp
           src/ui4d/DiscoveryProbe.cpp
          # New QuantumVerse Upgrade modules
          src/topological/DefectFactory.cpp
          src/holography/MERABuilder.cpp
          src/physics/GeodesicDeviation.cpp
          src/physics/GWPolarization.cpp
          src/exotic/ExoticMatterSynthesizer.cpp
          # Data components
                    src/data/SpacetimeExporter.cpp
                    src/data/LIGOAdapter.cpp
                    # Rendering components
                    src/rendering/OrbitRenderer.cpp
                    # Newtonian physics
                   src/physics/newtonian/NBodySimulator.cpp
            # N-body Dark Matter Simulation (Phase 3.3) - temporarily disabled due to pre-existing issues
            # src/nbody/Particle.cpp
            # src/nbody/NBodySimulator.cpp
            # Cosmology module
           src/cosmology/CosmicParameters.cpp
           src/cosmology/CosmicScaleFactor.cpp
           src/cosmology/FriedmannSolver.cpp
           src/cosmology/CMBCalculator.cpp
           src/cosmology/DarkEnergyModel.cpp
           # UI Cosmology Panel
            src/ui_imgui/CosmologyPanel.cpp
            # Quantum Cosmology Panel (Phase 5)
            src/ui_imgui/QuantumCosmologyPanel.cpp
            # Quantum Cosmology module (Phase 5)
            src/quantumcosmology/WheelerDeWittSolver.cpp
            src/quantumcosmology/LoopQuantumCosmology.cpp
            # Wavefunction Renderer (Phase 5)
            src/rendering/WavefunctionRenderer.cpp
            # Galaxy Formation module (Phase 6)
            src/astrophysics/StarFormation.cpp
            src/astrophysics/FeedbackManager.cpp
            src/astrophysics/GalaxyFormationModel.cpp
            # Galaxy Formation UI Panel (Phase 6)
            src/ui_imgui/GalaxyFormationPanel.cpp
            # Illustris Project module (Phase 7)
            src/astrophysics/AGNFeedback.cpp
            src/astrophysics/MetalEnrichment.cpp
            src/astrophysics/CoolingCurve.cpp
            src/astrophysics/StellarEvolution.cpp
            src/astrophysics/IllustrisGalaxyFormation.cpp
            # Illustris UI Panel (Phase 7)
            src/ui_imgui/IllustrisPanel.cpp
            # CFD module (Phase 7)
            src/hydro/EulerianGrid.cpp
            src/hydro/RiemannSolver.cpp
            src/hydro/AMRManager.cpp
            # CFD UI Panel (Phase 7)
            src/ui_imgui/CFDPanel.cpp
            # Astrophysics module (Phase 7)
            src/astrophysics/MHDSolver.cpp
            src/astrophysics/RadiationTransport.cpp
            src/astrophysics/StellarStructure.cpp
            # Astrophysics UI Panel (Phase 7)
            src/ui_imgui/AstrophysicsPanel.cpp
               )
          
          # UI ImGui components (conditional on IMGUI being enabled)
          if(QUANTUMVERSE_USE_IMGUI)
              target_sources(dilaton PRIVATE
                  src/ui_imgui/AccessibilityManager.cpp
                  src/ui_imgui/ContextMenu.cpp
                  src/ui_imgui/FabricMeshRenderer.cpp
                  src/ui_imgui/FlightTelemetryPanel.cpp
                  src/ui_imgui/FloatingPanels.cpp
                  src/ui_imgui/LayoutPresets.cpp
                  src/ui_imgui/MinimapPanel.cpp
                  src/ui_imgui/ObjectBrowser.cpp
                  src/ui_imgui/ObjectPicker.cpp
                  src/ui_imgui/OnboardingPanel.cpp
                  src/ui_imgui/PerformanceProfiler.cpp
                  src/ui_imgui/PropertyEditor.cpp
                  src/ui_imgui/ScriptInterface.cpp
                  src/ui_imgui/SpacetimeFlightController.cpp
                  src/ui_imgui/TheorySelectorPanel.cpp
                  src/ui_imgui/TimelineBar.cpp
                  src/ui_imgui/ToolPalette.cpp
                  src/ui_imgui/UndoRedoSystem.cpp
                  src/ui_imgui/VRIntegration.cpp
                  src/ui_imgui/ScreenshotCapture.cpp
                  src/ui_imgui/style/UIStyleManager.cpp
              )
          endif()

target_compile_features(dilaton PUBLIC cxx_std_17)

# Add AUTOMOC for Qt MOC support when QML is enabled
if(QUANTUMVERSE_USE_QML OR QUANTUMVERSE_USE_QT)
    set_target_properties(dilaton PROPERTIES AUTOMOC ON)
endif()

# Add ImGui core sources to dilaton when QUANTUMVERSE_USE_IMGUI is enabled
    # This is needed for tests that use ImGui UI components
    if(QUANTUMVERSE_USE_IMGUI)
        target_sources(dilaton PRIVATE
            third_party/imgui/imgui.cpp
            third_party/imgui/imgui_draw.cpp
            third_party/imgui/imgui_widgets.cpp
            third_party/imgui/imgui_tables.cpp
            third_party/imgui/imgui_demo.cpp
            third_party/imgui/backends/imgui_impl_glfw.cpp
        )
        target_include_directories(dilaton PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot
        )
        # Add GLFW include directory for imgui_impl_glfw.cpp
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64")
            target_include_directories(dilaton PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/include
            )
            target_link_directories(dilaton INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/lib-vc2022
            )
            target_link_libraries(dilaton INTERFACE glfw3)
        else()
            # GLFW already built via add_subdirectory above
            target_include_directories(dilaton PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
            )
            target_link_libraries(dilaton INTERFACE glfw)
        endif()
        target_link_libraries(dilaton INTERFACE OpenGL::GL)
    endif()
    
    # Link ONNX Runtime if enabled
    if(QUANTUMVERSE_USE_ONNX AND ONNX_RUNTIME_LIB)
        target_link_libraries(dilaton INTERFACE ${ONNX_RUNTIME_LIB})
    endif()

# Add glad for OpenGL support (needed by rendering tests)
find_package(OpenGL REQUIRED)
target_sources(dilaton PRIVATE third_party/glad/glad.cpp)

# ----------------------------------------------------------------------
# Qt QML GUI Executable
# ----------------------------------------------------------------------
if(QUANTUMVERSE_USE_QML OR QUANTUMVERSE_USE_QT)
    if(Qt6_FOUND)
        # Qt QML executable
         add_executable(quantumverse_qml
             src/main_qml.cpp
             src/qmlglviewport.cpp
             src/qmlcamcontroller.cpp
             src/scenegraphmodel.cpp
             src/ui4d/UI4D.cpp
             src/ui4d/Camera4D.cpp
             src/ui4d/Camera4DAdapter.cpp
             src/ui4d/SceneGraph.cpp
             src/ui4d/SceneGraphManager.cpp
             src/ui4d/PlanckMicroscope.cpp
             src/ui4d/EnhancedLayout.cpp
             src/ui4d/CausalGraph.cpp
             src/ui4d/DiscoveryProbe.cpp
             src/rendering/CurvatureRenderer.cpp
            src/rendering/QuantumGeometryRenderer.cpp
            src/rendering/CelestialBodyRenderer.cpp
            src/rendering/ProceduralTextures.cpp
            src/rendering/Texture.cpp
            src/quantumgravity/CDTEngine.cpp
            src/quantumgravity/CausalSet.cpp
            src/quantumgravity/SpinFoam.cpp
            src/quantumgravity/SpinNetwork.cpp
            src/quantumgravity/GFTEngine.cpp
            src/quantumgravity/SpinFoamEngine.cpp
            src/discovery/DiscoveryPanelManager.cpp
            src/discovery/DiscoveryEngine.cpp
            src/discovery/TheoryManager.cpp
            src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
            src/discovery/PatchedConicIntegrator.cpp
            src/discovery/PlanetaryGridGenerator.cpp
            src/discovery/GalacticRotationCurveScanner.cpp
            src/discovery/FineStructureConstantDriftObservatory.cpp
            src/discovery/BosonStarCollisionPredictor.cpp
            src/discovery/NeutronStarGlitchPhaseDetector.cpp
            src/discovery/UltralightDMWaveInterferometer.cpp
            src/discovery/BlackHoleJetAnomalyRecogniser.cpp
            src/discovery/PrimordialLithiumCrisisSolver.cpp
            src/discovery/GalacticTidalStreamCartographer.cpp
            src/discovery/RecombinationConstantVariationImager.cpp
            src/discovery/RLDiscoveryAgent.cpp
            src/discovery/SymbolicMath.cpp
            src/discovery/HolographicDualityLab.cpp
            src/ml/CurvatureNormalizingFlow.cpp
            src/ml/DifferentiableSimulator.cpp
            src/ml/GeodesicNeuralODE.cpp
            src/ml/MetricGNN.cpp
            third_party/glad/glad.cpp
            # Spacetime components
            src/spacetime/LightCone.cpp
            # Physics components
            src/physics/CurvatureCalculator.cpp
            # Utils components
             src/utils/ConfigManager.cpp
             src/utils/PerformanceMonitor.cpp
             src/utils/TraceLogger.cpp
             src/utils/CrashHandler.cpp
             src/rendering/GLDebug.cpp
             src/utils/DiagnosticMode.cpp
         )
        target_link_libraries(quantumverse_qml PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Qml
            Qt6::Quick
        )
        
        # Add resources
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/resources.qrc)
            qt6_add_resources(qt_resources src/resources.qrc)
            target_sources(quantumverse_qml PRIVATE ${qt_resources})
        endif()
        
        # Set MOC options
        set_target_properties(quantumverse_qml PROPERTIES
            AUTOMOC ON
            AUTORCC ON
            AUTOUIC ON
        )
        
        # Add compile definitions for the executable
        target_compile_definitions(quantumverse_qml PRIVATE
            QUANTUMVERSE_USE_QML
            QT_NO_NAMESPACE
        )
        
        # Force-include the Qt namespace fix header BEFORE any Qt headers
        # This is critical for Qt 6.11+ to prevent namespace auto-detection
        if(MSVC)
            target_compile_options(quantumverse_qml PRIVATE
                /FI${CMAKE_CURRENT_SOURCE_DIR}/src/qt_namespace_fix.h
            )
        else()
            target_compile_options(quantumverse_qml PRIVATE
                -include${CMAKE_CURRENT_SOURCE_DIR}/src/qt_namespace_fix.h
            )
        endif()
    endif()
endif()

# ----------------------------------------------------------------------
# Unit Tests
# ----------------------------------------------------------------------
enable_testing()

# Dilaton black hole tests (Catch2 framework)
add_executable(test_dilaton tests/test_dilaton_black_hole.cpp)
target_link_libraries(test_dilaton PRIVATE dilaton)
add_test(NAME DilatonTest COMMAND test_dilaton)

# Add glad for OpenGL support in tests
if(QUANTUMVERSE_USE_IMGUI OR QUANTUMVERSE_USE_QML)
    target_sources(dilaton PRIVATE third_party/glad/glad.cpp)
endif()

# Additional dilaton tests (assert-based)
add_executable(test_dilaton_blackhole tests/test_dilaton_blackhole.cpp)
target_link_libraries(test_dilaton_blackhole PRIVATE dilaton)
add_test(NAME DilatonBlackHoleTest COMMAND test_dilaton_blackhole)

# Hawking calculator tests (assert-based)
add_executable(test_hawking_calculator tests/test_hawking_calculator.cpp)
target_link_libraries(test_hawking_calculator PRIVATE dilaton)
add_test(NAME HawkingCalculatorTest COMMAND test_hawking_calculator)

# Curvature renderer tests (assert-based)
add_executable(test_curvature_renderer tests/test_curvature_renderer.cpp)
target_link_libraries(test_curvature_renderer PRIVATE dilaton)
add_test(NAME CurvatureRendererTest COMMAND test_curvature_renderer)

# Anomaly detection tests (assert-based)
add_executable(test_anomaly_detection tests/test_anomaly_detection.cpp)
target_link_libraries(test_anomaly_detection PRIVATE dilaton)
add_test(NAME AnomalyDetectionTest COMMAND test_anomaly_detection)

# ----------------------------------------------------------------------
# GR Validation Tests
# ----------------------------------------------------------------------
# Mercury precession test
add_executable(test_mercury_precession tests/test_mercury_precession.cpp)
target_link_libraries(test_mercury_precession PRIVATE dilaton)
add_test(NAME MercuryPrecessionTest COMMAND test_mercury_precession)

# Light deflection test
add_executable(test_light_deflection tests/test_light_deflection.cpp)
target_link_libraries(test_light_deflection PRIVATE dilaton)
add_test(NAME LightDeflectionTest COMMAND test_light_deflection)

# Gravitational redshift test
add_executable(test_gravitational_redshift tests/test_gravitational_redshift.cpp)
target_link_libraries(test_gravitational_redshift PRIVATE dilaton)
add_test(NAME GravitationalRedshiftTest COMMAND test_gravitational_redshift)

# Frame dragging test
add_executable(test_frame_dragging tests/test_frame_dragging.cpp)
target_link_libraries(test_frame_dragging PRIVATE dilaton)
add_test(NAME FrameDraggingTest COMMAND test_frame_dragging)

# Lunar Laser Ranging test (Nordtvedt parameter)
add_executable(test_lunar_laser_ranging tests/test_lunar_laser_ranging.cpp)
target_link_libraries(test_lunar_laser_ranging PRIVATE dilaton)
add_test(NAME LunarLaserRangingTest COMMAND test_lunar_laser_ranging)

# TOV Limit test (Neutron star mass-radius)
add_executable(test_tov_limit tests/test_tov_limit.cpp)
target_link_libraries(test_tov_limit PRIVATE dilaton)
add_test(NAME TOVLimitTest COMMAND test_tov_limit)

# ----------------------------------------------------------------------
# Validation Tests (Phase QA)
# ----------------------------------------------------------------------
# Shock Tube (Sod) test - 1D Riemann problem
add_executable(test_shock_tube tests/validation/test_shock_tube.cpp)
target_link_libraries(test_shock_tube PRIVATE dilaton)
add_test(NAME ShockTubeTest COMMAND test_shock_tube)

# Sedov Blast test - point explosion in uniform medium
add_executable(test_sedov tests/validation/test_sedov.cpp)
target_link_libraries(test_sedov PRIVATE dilaton)
add_test(NAME SedovBlastTest COMMAND test_sedov)

# MRI test - magnetorotational instability growth
add_executable(test_mri tests/validation/test_mri.cpp)
target_link_libraries(test_mri PRIVATE dilaton)
add_test(NAME MRITest COMMAND test_mri)

# Lane-Emden test - polytrope solutions (n=0,1,5)
add_executable(test_lane_emden tests/validation/test_lane_emden.cpp)
target_link_libraries(test_lane_emden PRIVATE dilaton)
add_test(NAME LaneEmdenTest COMMAND test_lane_emden)

# ----------------------------------------------------------------------
# Additional Module Tests
# ----------------------------------------------------------------------
# MetricGNN test
add_executable(test_metric_gnn tests/test_metric_gnn.cpp)
target_link_libraries(test_metric_gnn PRIVATE dilaton)
add_test(NAME MetricGNNTest COMMAND test_metric_gnn)

# Bayesian Evidence test
add_executable(test_bayesian_evidence tests/test_bayesian_evidence.cpp)
target_link_libraries(test_bayesian_evidence PRIVATE dilaton)
add_test(NAME BayesianEvidenceTest COMMAND test_bayesian_evidence)

# Paper Generator test
add_executable(test_paper_generator tests/test_paper_generator.cpp)
target_link_libraries(test_paper_generator PRIVATE dilaton)
add_test(NAME PaperGeneratorTest COMMAND test_paper_generator)

# VR Multiplayer test
add_executable(test_vr_multiplayer tests/test_vr_multiplayer.cpp)
target_link_libraries(test_vr_multiplayer PRIVATE dilaton)
add_test(NAME VRMutliplayerTest COMMAND test_vr_multiplayer)

# RL Discovery Agent test
add_executable(test_rl_discovery tests/test_rl_discovery.cpp)
target_link_libraries(test_rl_discovery PRIVATE dilaton)
add_test(NAME RLDiscoveryTest COMMAND test_rl_discovery)

# Symbolic Regression test
add_executable(test_symbolic_regression tests/test_symbolic_regression.cpp)
target_link_libraries(test_symbolic_regression PRIVATE dilaton)
add_test(NAME SymbolicRegressionTest COMMAND test_symbolic_regression)

# Curvature Flow test
add_executable(test_curvature_flow tests/test_curvature_flow.cpp)
target_link_libraries(test_curvature_flow PRIVATE dilaton)
add_test(NAME CurvatureFlowTest COMMAND test_curvature_flow)

# Crash Guard regression tests
add_executable(test_crash_guard tests/CrashGuardTests.cpp)
target_link_libraries(test_crash_guard PRIVATE dilaton)
add_test(NAME CrashGuardTest COMMAND test_crash_guard)

# Physics Validator tests
add_executable(test_physics_validator tests/test_physics_validator.cpp)
target_link_libraries(test_physics_validator PRIVATE dilaton)
add_test(NAME PhysicsValidatorTest COMMAND test_physics_validator)

# Memory Safety tests
add_executable(test_memory_safety tests/test_memory_safety.cpp)
target_link_libraries(test_memory_safety PRIVATE dilaton)
add_test(NAME MemorySafetyTest COMMAND test_memory_safety)

# ----------------------------------------------------------------------
# Advanced Error Discovery Tests
# ----------------------------------------------------------------------
# CBMC verification tests
add_executable(test_cbmc_verification tests/test_cbmc_verification.cpp)
target_link_libraries(test_cbmc_verification PRIVATE dilaton)
add_test(NAME CBMCVerificationTest COMMAND test_cbmc_verification)

# Differential testing
add_executable(test_differential tests/test_differential.cpp)
target_link_libraries(test_differential PRIVATE dilaton)
add_test(NAME DifferentialTest COMMAND test_differential)

# Symbolic execution tests
add_executable(test_symbolic_execution tests/test_symbolic_execution.cpp)
target_link_libraries(test_symbolic_execution PRIVATE dilaton)
add_test(NAME SymbolicExecutionTest COMMAND test_symbolic_execution)

# ----------------------------------------------------------------------
# CBMC Formal Verification Harness Tests
# ----------------------------------------------------------------------
if(QUANTUMVERSE_ENABLE_CBMC)
    message(STATUS "CBMC verification harnesses enabled")
    
    add_executable(cbmc_harness_metric tests/verification/harness_metric.cpp)
    target_link_libraries(cbmc_harness_metric PRIVATE dilaton)
    target_compile_definitions(cbmc_harness_metric PRIVATE CBMC_VERIFY)
    add_test(NAME CBMCHarnessMetric COMMAND cbmc_harness_metric)
    
    add_executable(cbmc_harness_geodesic tests/verification/harness_geodesic.cpp)
    target_link_libraries(cbmc_harness_geodesic PRIVATE dilaton)
    target_compile_definitions(cbmc_harness_geodesic PRIVATE CBMC_VERIFY)
    add_test(NAME CBMCHarnessGeodesic COMMAND cbmc_harness_geodesic)
endif()

# ----------------------------------------------------------------------
# Golden Master Tests
# ----------------------------------------------------------------------
if(QUANTUMVERSE_USE_GOLDEN_MASTER)
    message(STATUS "Golden master tests enabled")
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
        add_test(NAME GoldenMasterTest 
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/golden_master_test.py EarthRenderer
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()

# ----------------------------------------------------------------------
# Quantum Gravity Engine Tests
# ----------------------------------------------------------------------
# CDT (Causal Dynamical Triangulations) test
add_executable(test_cdt tests/test_cdt.cpp)
target_link_libraries(test_cdt PRIVATE dilaton)
add_test(NAME CDTTest COMMAND test_cdt)

# CDT Convergence test - validates spectral dimension convergence
add_executable(test_cdt_convergence tests/test_cdt_convergence.cpp)
target_link_libraries(test_cdt_convergence PRIVATE dilaton)
add_test(NAME CDTConvergenceTest COMMAND test_cdt_convergence)

# Spin Foam (Loop Quantum Gravity) test
add_executable(test_spin_foam tests/test_spin_foam.cpp)
target_link_libraries(test_spin_foam PRIVATE dilaton)
add_test(NAME SpinFoamTest COMMAND test_spin_foam)

# GFT (Group Field Theory) test
add_executable(test_gft tests/test_gft.cpp)
target_link_libraries(test_gft PRIVATE dilaton)
add_test(NAME GFTTest COMMAND test_gft)

# Causal Set test
add_executable(test_causal_set tests/test_causal_set.cpp)
target_link_libraries(test_causal_set PRIVATE dilaton)
add_test(NAME CausalSetTest COMMAND test_causal_set)

# ----------------------------------------------------------------------
# Additional Physics Tests
# ----------------------------------------------------------------------
# Differentiable Geodesic test
add_executable(test_differentiable_geodesic tests/test_differentiable_geodesic.cpp)
target_link_libraries(test_differentiable_geodesic PRIVATE dilaton)
add_test(NAME DifferentiableGeodesicTest COMMAND test_differentiable_geodesic)

# Differentiable Curvature test
add_executable(test_differentiable_curvature tests/test_differentiable_curvature.cpp)
target_link_libraries(test_differentiable_curvature PRIVATE dilaton)
add_test(NAME DifferentiableCurvatureTest COMMAND test_differentiable_curvature)

# Differentiable AD test
add_executable(test_differentiable_ad tests/test_differentiable_ad.cpp)
target_link_libraries(test_differentiable_ad PRIVATE dilaton)
add_test(NAME DifferentiableADTest COMMAND test_differentiable_ad)

# Differentiable Benchmark test
add_executable(test_differentiable_benchmark tests/test_differentiable_benchmark.cpp)
target_link_libraries(test_differentiable_benchmark PRIVATE dilaton)
add_test(NAME DifferentiableBenchmarkTest COMMAND test_differentiable_benchmark)

# Differentiable Simulator test
add_executable(test_differentiable_simulator tests/test_differentiable_simulator.cpp)
target_link_libraries(test_differentiable_simulator PRIVATE dilaton)
add_test(NAME DifferentiableSimulatorTest COMMAND test_differentiable_simulator)

# Cosmology tests
add_executable(test_cosmology tests/test_cosmology.cpp)
target_link_libraries(test_cosmology PRIVATE dilaton)
add_test(NAME CosmologyTest COMMAND test_cosmology)

# Cosmology integration tests
add_executable(test_cosmology_integration tests/test_cosmology_integration.cpp)
target_link_libraries(test_cosmology_integration PRIVATE dilaton)
add_test(NAME CosmologyIntegrationTest COMMAND test_cosmology_integration)

# FRW Cosmology test
add_executable(test_frw_cosmology tests/test_frw_cosmology.cpp)
target_link_libraries(test_frw_cosmology PRIVATE dilaton)
add_test(NAME FRWCosmologyTest COMMAND test_frw_cosmology)

# Light Cone test
add_executable(test_lightcone tests/test_lightcone.cpp)
target_link_libraries(test_lightcone PRIVATE dilaton)
add_test(NAME LightConeTest COMMAND test_lightcone)

# ----------------------------------------------------------------------
# Additional Core Tests (missing from initial registration)
# ----------------------------------------------------------------------
# AutoDiff tests
add_executable(test_autodiff tests/test_autodiff.cpp)
target_link_libraries(test_autodiff PRIVATE dilaton)
add_test(NAME AutoDiffTest COMMAND test_autodiff)

# Event4D tests
add_executable(test_event4d tests/test_event4d.cpp)
target_link_libraries(test_event4d PRIVATE dilaton)
add_test(NAME Event4DTest COMMAND test_event4d)

# Matrix4x4 tests
add_executable(test_matrix4x4 tests/test_matrix4x4.cpp)
target_link_libraries(test_matrix4x4 PRIVATE dilaton)
add_test(NAME Matrix4x4Test COMMAND test_matrix4x4)

# Vector4D tests
add_executable(test_vector4d tests/test_vector4d.cpp)
target_link_libraries(test_vector4d PRIVATE dilaton)
add_test(NAME Vector4DTest COMMAND test_vector4d)

# SO4Rotation tests
add_executable(test_so4_rotation tests/test_so4_rotation.cpp)
target_link_libraries(test_so4_rotation PRIVATE dilaton)
add_test(NAME SO4RotationTest COMMAND test_so4_rotation)

# Geodesic tests
add_executable(test_geodesic tests/test_geodesic.cpp)
target_link_libraries(test_geodesic PRIVATE dilaton)
add_test(NAME GeodesicTest COMMAND test_geodesic)

# Gravity tests
add_executable(test_gravity tests/test_gravity.cpp)
target_link_libraries(test_gravity PRIVATE dilaton)
add_test(NAME GravityTest COMMAND test_gravity)

# Relativity tests
add_executable(test_relativity tests/test_relativity.cpp)
target_link_libraries(test_relativity PRIVATE dilaton)
add_test(NAME RelativityTest COMMAND test_relativity)

# Spacetime tests
add_executable(test_spacetime tests/test_spacetime.cpp)
target_link_libraries(test_spacetime PRIVATE dilaton)
add_test(NAME SpacetimeTest COMMAND test_spacetime)

# Integration tests
add_executable(test_integration tests/test_integration.cpp)
target_link_libraries(test_integration PRIVATE dilaton)
add_test(NAME IntegrationTest COMMAND test_integration)

# Discovery tests
add_executable(test_discovery tests/test_discovery.cpp)
target_link_libraries(test_discovery PRIVATE dilaton)
add_test(NAME DiscoveryTest COMMAND test_discovery)

# Theory plugins tests
add_executable(test_theory_plugins tests/test_theory_plugins.cpp)
target_link_libraries(test_theory_plugins PRIVATE dilaton)
add_test(NAME TheoryPluginsTest COMMAND test_theory_plugins)

# LIGO adapter tests
add_executable(test_ligo_adapter tests/test_ligo_adapter.cpp)
target_link_libraries(test_ligo_adapter PRIVATE dilaton)
add_test(NAME LIGOAdapterTest COMMAND test_ligo_adapter)

# Neural ODE tests
add_executable(test_neural_ode tests/test_neural_ode.cpp)
target_link_libraries(test_neural_ode PRIVATE dilaton)
add_test(NAME NeuralODETest COMMAND test_neural_ode)

# Rendering tests
add_executable(test_orbit_renderer tests/test_orbit_renderer.cpp)
target_link_libraries(test_orbit_renderer PRIVATE dilaton)
add_test(NAME OrbitRendererTest COMMAND test_orbit_renderer)

# Texture tests
add_executable(test_texture tests/test_texture.cpp)
target_link_libraries(test_texture PRIVATE dilaton)
add_test(NAME TextureTest COMMAND test_texture)

# Solar system tests
add_executable(test_solar_system tests/test_solar_system.cpp)
target_link_libraries(test_solar_system PRIVATE dilaton)
add_test(NAME SolarSystemTest COMMAND test_solar_system)

# Validation tests
add_executable(test_validation tests/test_validation.cpp)
target_link_libraries(test_validation PRIVATE dilaton)
add_test(NAME ValidationTest COMMAND test_validation)

# Validation harness tests (Phase QA)
add_executable(test_validation_harness tests/test_validation_harness.cpp)
target_link_libraries(test_validation_harness PRIVATE dilaton)
add_test(NAME ValidationHarnessTest COMMAND test_validation_harness)

# Physics integration tests
add_executable(test_physics_integration tests/test_physics_integration.cpp)
target_link_libraries(test_physics_integration PRIVATE dilaton)
add_test(NAME PhysicsIntegrationTest COMMAND test_physics_integration)

# MetricTensor tests
add_executable(test_metrictensor tests/test_metrictensor.cpp)
target_link_libraries(test_metrictensor PRIVATE dilaton)
add_test(NAME MetricTensorTest COMMAND test_metrictensor)

# Curvature Schwarzschild tests
add_executable(test_curvature_schwarzschild tests/test_curvature_schwarzschild.cpp)
target_link_libraries(test_curvature_schwarzschild PRIVATE dilaton)
add_test(NAME CurvatureSchwarzschildTest COMMAND test_curvature_schwarzschild)

# Singularity tests
add_executable(test_singularity tests/test_singularity.cpp)
target_link_libraries(test_singularity PRIVATE dilaton)
add_test(NAME SingularityTest COMMAND test_singularity)

# API Contract tests
add_executable(test_api_contract tests/test_api_contract.cpp)
target_link_libraries(test_api_contract PRIVATE dilaton)
add_test(NAME ApiContractTest COMMAND test_api_contract)

# N-body Phase 3.3 tests
# Temporarily disabled - depends on N-body classes that are not yet implemented
# add_executable(test_nbody_phase3 tests/test_nbody_phase3.cpp)
# target_link_libraries(test_nbody_phase3 PRIVATE dilaton)
# add_test(NAME NBodyPhase3Test COMMAND test_nbody_phase3)

# Quantum Cosmology tests (Phase 5)
add_executable(test_quantum_cosmology tests/test_quantum_cosmology.cpp)
target_link_libraries(test_quantum_cosmology PRIVATE dilaton)
add_test(NAME QuantumCosmologyTest COMMAND test_quantum_cosmology)

# Galaxy Formation tests (Phase 6)
add_executable(test_galaxy_formation tests/test_galaxy_formation.cpp)
target_link_libraries(test_galaxy_formation PRIVATE dilaton)
add_test(NAME GalaxyFormationTest COMMAND test_galaxy_formation)

# ----------------------------------------------------------------------
# Integration Tests
# ----------------------------------------------------------------------
# Headless UI Integration Test (uses quantumverse_imgui executable)
add_test(NAME HeadlessUITest COMMAND quantumverse_imgui --headless --frames 50)

# System Scenario Tests (core physics components)
add_executable(test_solar_system_tour tests/integration/test_solar_system_tour.cpp)
target_link_libraries(test_solar_system_tour PRIVATE dilaton)
add_test(NAME SolarSystemTourTest COMMAND test_solar_system_tour)

add_executable(test_black_hole_exploration tests/integration/test_black_hole_exploration.cpp)
target_link_libraries(test_black_hole_exploration PRIVATE dilaton)
add_test(NAME BlackHoleExplorationTest COMMAND test_black_hole_exploration)

# ----------------------------------------------------------------------
# Performance Tests
# ----------------------------------------------------------------------
add_executable(test_stress_performance tests/performance/test_stress_performance.cpp)
target_link_libraries(test_stress_performance PRIVATE dilaton)
add_test(NAME StressPerformanceTest COMMAND test_stress_performance --iterations 10000)

# ----------------------------------------------------------------------
# Integration Tests - Module Pair Verification
# ----------------------------------------------------------------------
# Spacetime-Physics: Metric and Geodesic integration
add_executable(test_metric_geodesic tests/integration/test_metric_geodesic.cpp)
target_link_libraries(test_metric_geodesic PRIVATE dilaton)
add_test(NAME MetricGeodesicTest COMMAND test_metric_geodesic)

# Spacetime-Rendering: Curvature calculation for visualization
add_executable(test_curvature_calculator tests/integration/test_curvature_calculator.cpp)
target_link_libraries(test_curvature_calculator PRIVATE dilaton)
add_test(NAME CurvatureCalculatorTest COMMAND test_curvature_calculator)

# UI4D-Rendering: UI actions affect rendering
add_executable(test_ui_rendering tests/integration/test_ui_rendering.cpp)
target_link_libraries(test_ui_rendering PRIVATE dilaton)
add_test(NAME UIRenderingTest COMMAND test_ui_rendering)

# UI4D-Discovery: Discovery panel updates as simulation state changes
add_executable(test_ui_discovery tests/integration/test_ui_discovery.cpp)
target_link_libraries(test_ui_discovery PRIVATE dilaton)
add_test(NAME UIDiscoveryTest COMMAND test_ui_discovery)

# Discovery-QuantumGravity: Theory plugins produce expected outputs
add_executable(test_discovery_qg tests/integration/test_discovery_qg.cpp)
target_link_libraries(test_discovery_qg PRIVATE dilaton)
add_test(NAME DiscoveryQGTest COMMAND test_discovery_qg)

# Data-Physics: Data adapters against simulated waveforms
add_executable(test_data_physics tests/integration/test_data_physics.cpp)
target_link_libraries(test_data_physics PRIVATE dilaton)
add_test(NAME DataPhysicsTest COMMAND test_data_physics)

# ML-Physics: Neural ODE surrogate matches RK4 within tolerance
add_executable(test_ml_physics tests/integration/test_ml_physics.cpp)
target_link_libraries(test_ml_physics PRIVATE dilaton)
add_test(NAME MLPhysicsTest COMMAND test_ml_physics)

# ============================================================================
# Phase 2: System Scenarios
add_executable(test_scenarios 
    tests/scenarios/test_scenarios.cpp
    tests/scenarios/ScenarioRunner.cpp
)
target_link_libraries(test_scenarios PRIVATE dilaton)
add_test(NAME SystemScenariosTest COMMAND test_scenarios)

# Discovery Instruments Test
add_executable(test_discovery_instruments tests/integration/test_discovery_instruments.cpp)
target_link_libraries(test_discovery_instruments PRIVATE dilaton)
add_test(NAME DiscoveryInstrumentsTest COMMAND test_discovery_instruments)

# Discovery Instruments Full Tests
add_executable(test_discovery_instruments_full tests/integration/test_discovery_instruments_full.cpp)
target_link_libraries(test_discovery_instruments_full PRIVATE dilaton)
add_test(NAME DiscoveryInstrumentsFullTest COMMAND test_discovery_instruments_full)

# Discovery Module Unit Tests
add_executable(test_discovery_module tests/discovery/test_discovery_instruments.cpp)
target_link_libraries(test_discovery_module PRIVATE dilaton)
add_test(NAME DiscoveryModuleTest COMMAND test_discovery_module)

# Discovery Synthetic Tests
add_executable(test_discovery_synthetic tests/discovery/test_exoplanetary_hunter.cpp)
target_link_libraries(test_discovery_synthetic PRIVATE dilaton)
add_test(NAME DiscoverySyntheticTest COMMAND test_discovery_synthetic)

# Additional Discovery Instrument Tests
add_executable(test_galactic_rotation tests/discovery/test_galactic_rotation.cpp)
target_link_libraries(test_galactic_rotation PRIVATE dilaton)
add_test(NAME GalacticRotationTest COMMAND test_galactic_rotation)

add_executable(test_fine_structure tests/discovery/test_fine_structure_drift.cpp)
target_link_libraries(test_fine_structure PRIVATE dilaton)
add_test(NAME FineStructureTest COMMAND test_fine_structure)

add_executable(test_boson_star tests/discovery/test_boson_star.cpp)
target_link_libraries(test_boson_star PRIVATE dilaton)
add_test(NAME BosonStarTest COMMAND test_boson_star)

add_executable(test_neutron_star tests/discovery/test_neutron_star.cpp)
target_link_libraries(test_neutron_star PRIVATE dilaton)
add_test(NAME NeutronStarTest COMMAND test_neutron_star)

# Stress & Chaos Tests
add_executable(test_chaos tests/stress/test_chaos.cpp)
target_link_libraries(test_chaos PRIVATE dilaton)
add_test(NAME ChaosTest COMMAND test_chaos)

# Telemetry/Observability
add_library(telemetry src/utils/Telemetry.cpp)
target_include_directories(telemetry PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Property-Based Tests (rapidcheck)
option(QUANTUMVERSE_USE_RAPIDCHECK "Build property-based tests" ON)

if(QUANTUMVERSE_USE_RAPIDCHECK)
    find_package(RapidCheck QUIET)
    if(RapidCheck_FOUND)
        message(STATUS "RapidCheck found - building property-based tests")
        
        add_executable(test_property_based tests/test_property_based.cpp)
        target_link_libraries(test_property_based PRIVATE dilaton RapidCheck::RapidCheck)
        add_test(NAME PropertyBasedTest COMMAND test_property_based)
    else()
        message(WARNING "RapidCheck not found - skipping property-based tests. Install via: vcpkg install rapidcheck")
    endif()
endif()

# Formal Verification Tests (high-precision)
option(QUANTUMVERSE_USE_BOOST_MULTIPRECISION "Build formal verification tests" OFF)

if(QUANTUMVERSE_USE_BOOST_MULTIPRECISION)
    find_package(Boost QUIET)
    if(Boost_FOUND)
        message(STATUS "Boost found - building formal verification tests")
        
        add_executable(test_formal_verification tests/test_formal_verification.cpp)
        target_link_libraries(test_formal_verification PRIVATE dilaton)
        add_test(NAME FormalVerificationTest COMMAND test_formal_verification)
    else()
        message(WARNING "Boost not found - skipping formal verification tests")
    endif()
endif()

# ----------------------------------------------------------------------
# Future: Full project targets (to be added)
# ----------------------------------------------------------------------
# - Core library (spacetime, physics, math, discovery, quantumgravity, rendering)
# - Executables: quantumverse_qt (with Qt), quantumverse_console
# - Additional tests for all modules
# - Python bindings (via pybind11)
# - Documentation (Doxygen)

# Note for developers: To build and run the dilaton tests:
#   mkdir -p build && cd build
#   cmake .. -DCMAKE_BUILD_TYPE=Release
#   cmake --build . --config Release --parallel
#   ctest -R DilatonTest --output-on-failure

# ----------------------------------------------------------------------
# Fuzzing Target (libFuzzer/AFL)
# ----------------------------------------------------------------------
option(QUANTUMVERSE_USE_FUZZER "Build fuzzing targets" OFF)

if(QUANTUMVERSE_USE_FUZZER)
    # libFuzzer requires Clang
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(WARNING "Fuzzer target requires Clang compiler")
    else()
        message(STATUS "Building fuzzing targets")
        
        add_executable(fuzz_metric
            tests/fuzz_metric.cpp
        )
        
        target_link_libraries(fuzz_metric PRIVATE dilaton)
        
        # Link against libFuzzer
        target_link_options(fuzz_metric PRIVATE -fsanitize=fuzzer)
        
        # Add compile options for fuzzing
        target_compile_options(fuzz_metric PRIVATE
            -fsanitize=fuzzer-no-link
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        
        add_test(NAME FuzzMetricTest COMMAND fuzz_metric)
        
        # API contract fuzzer
        add_executable(fuzz_api_contract
            tests/fuzz_api_contract.cpp
        )
        
        target_link_libraries(fuzz_api_contract PRIVATE dilaton)
        
        target_link_options(fuzz_api_contract PRIVATE -fsanitize=fuzzer)
        
        target_compile_options(fuzz_api_contract PRIVATE
            -fsanitize=fuzzer-no-link
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        
        add_test(NAME FuzzApiContractTest COMMAND fuzz_api_contract)
        
        # Geodesic fuzzer
        add_executable(fuzz_geodesic
            tests/fuzz_geodesic.cpp
        )
        
        target_link_libraries(fuzz_geodesic PRIVATE dilaton)
        
        target_link_options(fuzz_geodesic PRIVATE -fsanitize=fuzzer)
        
        target_compile_options(fuzz_geodesic PRIVATE
            -fsanitize=fuzzer-no-link
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        
        add_test(NAME FuzzGeodesicTest COMMAND fuzz_geodesic)
        
        # ONNX fuzzer
        add_executable(fuzz_onnx
            tests/fuzz_onnx.cpp
        )
        
        target_link_libraries(fuzz_onnx PRIVATE dilaton)
        
        target_link_options(fuzz_onnx PRIVATE -fsanitize=fuzzer)
        
        target_compile_options(fuzz_onnx PRIVATE
            -fsanitize=fuzzer-no-link
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        
        add_test(NAME FuzzOnnxTest COMMAND fuzz_onnx)
        
        # Event4D fuzzer
        add_executable(fuzz_event4d
            tests/fuzz_event4d.cpp
        )
        
        target_link_libraries(fuzz_event4d PRIVATE dilaton)
        
        target_link_options(fuzz_event4d PRIVATE -fsanitize=fuzzer)
        
        target_compile_options(fuzz_event4d PRIVATE
            -fsanitize=fuzzer-no-link
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        
        add_test(NAME FuzzEvent4DTest COMMAND fuzz_event4d)
        
        # Differential fuzzer - compares C++ vs Python surrogate
        add_executable(fuzz_differential
            tests/fuzz_differential.cpp
        )
        
        target_link_libraries(fuzz_differential PRIVATE dilaton)
        
        target_link_options(fuzz_differential PRIVATE -fsanitize=fuzzer)
        
        target_compile_options(fuzz_differential PRIVATE
            -fsanitize=fuzzer-no-link
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        
        add_test(NAME FuzzDifferentialTest COMMAND fuzz_differential)
    endif()
endif()

# ----------------------------------------------------------------------
# GoogleMock Tests (for renderer mocking)
# ----------------------------------------------------------------------
option(QUANTUMVERSE_USE_GMOCK "Build GoogleMock tests" OFF)

if(QUANTUMVERSE_USE_GMOCK)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "GoogleMock found - building mock tests")
        
        add_executable(test_mock_renderer
            tests/MockRendererTests.cpp
        )
        
        target_link_libraries(test_mock_renderer PRIVATE
            dilaton
            GTest::gtest
            GTest::gmock
        )
        
        add_test(NAME MockRendererTest COMMAND test_mock_renderer)
    else()
        message(WARNING "GoogleMock not found - skipping mock tests")
    endif()
endif()

# ----------------------------------------------------------------------
# Qt Test (for QML components)
# ----------------------------------------------------------------------
if(QUANTUMVERSE_USE_QML AND Qt6_FOUND)
    find_package(Qt6Test QUIET)
    if(Qt6Test_FOUND)
        message(STATUS "Qt6 Test found - building QML tests")
        
        add_executable(test_qml_components
            tests/test_qml_components.cpp
        )
        
        target_link_libraries(test_qml_components PRIVATE
            dilaton
            Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            Qt6::Test
        )
        
        add_test(NAME QMLComponentsTest COMMAND test_qml_components)
    else()
        message(WARNING "Qt6 Test not found - skipping QML tests")
    endif()
endif()

# ----------------------------------------------------------------------
# ImGui Tests (Catch2 framework)
# ----------------------------------------------------------------------
if(QUANTUMVERSE_USE_IMGUI)
    add_executable(test_imgui_components
        tests/test_imgui_components.cpp
    )
    
    target_link_libraries(test_imgui_components PRIVATE dilaton)
    add_test(NAME ImGuiComponentsTest COMMAND test_imgui_components)
endif()

# ImGui UI tests (assert-based)
# NOTE: test_imgui_ui is disabled - it requires linking to the same sources as quantumverse_imgui
# which is an executable. The test can be re-enabled when a proper library structure is established.
if(QUANTUMVERSE_USE_IMGUI)
    # add_executable(test_imgui_ui
    #     tests/test_imgui_ui.cpp
    # )
    # 
    # target_link_libraries(test_imgui_ui PRIVATE quantumverse_imgui)
    # add_test(NAME ImGuiUITest COMMAND test_imgui_ui)
endif()

# ----------------------------------------------------------------------
# GUI Automation Tests
# ----------------------------------------------------------------------
# GUI automation tests using ImGui Test Engine (requires imgui_test_engine)
# To enable, clone https://github.com/ocornut/imgui_test_engine into third_party/
option(QUANTUMVERSE_USE_IMGUI_TEST_ENGINE "Build GUI automation tests with ImGui Test Engine" OFF)

if(QUANTUMVERSE_USE_IMGUI_TEST_ENGINE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui_test_engine")
        message(STATUS "ImGui Test Engine found - building GUI automation tests")
        
        add_subdirectory(third_party/imgui_test_engine)
        
        add_executable(test_gui_automation
            tests/test_gui_automation.cpp
        )
        
        target_link_libraries(test_gui_automation PRIVATE
            dilaton
            imgui_test_engine
        )
        
        add_test(NAME GUIAutomationTest COMMAND test_gui_automation)
    else()
        message(WARNING "ImGui Test Engine not found - building standalone GUI automation test")
        message(STATUS "To enable full automation, clone: https://github.com/ocornut/imgui_test_engine")
        
        add_executable(test_gui_automation
            tests/test_gui_automation.cpp
        )
        
        target_link_libraries(test_gui_automation PRIVATE dilaton)
        add_test(NAME GUIAutomationTest COMMAND test_gui_automation)
    endif()
else()
    # Build standalone GUI test without Test Engine
    add_executable(test_gui_automation
        tests/test_gui_automation.cpp
    )
    
    target_link_libraries(test_gui_automation PRIVATE dilaton)
    add_test(NAME GUIAutomationTest COMMAND test_gui_automation)
endif()

# ----------------------------------------------------------------------
# Minimal GUI Tests (for crash isolation)
# ----------------------------------------------------------------------
# Minimal QML test - tests Qt startup without any application code
if(QUANTUMVERSE_USE_QML AND Qt6_FOUND)
    add_executable(minimal_qml tests/minimal_gui_test.cpp)
    target_link_libraries(minimal_qml PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    )
    target_compile_definitions(minimal_qml PRIVATE TEST_QML)
endif()

# Minimal ImGui test - tests GLFW/OpenGL startup without any application code
if(QUANTUMVERSE_USE_IMGUI)
    add_executable(minimal_imgui tests/minimal_gui_test.cpp)
    # Add GLFW include and link directories (same as quantumverse_imgui)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64")
        target_include_directories(minimal_imgui PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/include
        )
        target_link_directories(minimal_imgui PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/lib-vc2022
        )
        target_link_libraries(minimal_imgui PRIVATE glfw3)
    else()
        # GLFW already built via add_subdirectory above
        target_link_libraries(minimal_imgui PRIVATE glfw)
    endif()
    target_link_libraries(minimal_imgui PRIVATE
        OpenGL::GL
    )
    target_compile_definitions(minimal_imgui PRIVATE TEST_IMGUI)
endif()

# Minimal OpenGL test - tests raw OpenGL rendering without ImGui
if(QUANTUMVERSE_USE_IMGUI)
    add_executable(test_opengl_minimal tests/test_opengl_minimal.cpp)
    # Add GLFW include and link directories (same as quantumverse_imgui)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64")
        target_include_directories(test_opengl_minimal PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/include
        )
        target_link_directories(test_opengl_minimal PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.4.bin.WIN64/lib-vc2022
        )
        target_link_libraries(test_opengl_minimal PRIVATE glfw3)
    else()
        # GLFW already built via add_subdirectory above
        target_link_libraries(test_opengl_minimal PRIVATE glfw)
    endif()
    target_link_libraries(test_opengl_minimal PRIVATE
        OpenGL::GL
    )
    # Add glad source for OpenGL function loading
    target_sources(test_opengl_minimal PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/glad.cpp
    )
endif()

# ----------------------------------------------------------------------
# Error Report Target
# ----------------------------------------------------------------------
option(QUANTUMVERSE_GENERATE_REPORT "Generate error detection report" OFF)

if(QUANTUMVERSE_GENERATE_REPORT)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    
    if(Python3_FOUND)
        add_custom_target(report
            COMMAND ${CMAKE_COMMAND} -E echo "Generating error report..."
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_error_report.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running error detection and generating report"
        )
    else()
        # Fallback to shell script on Unix, batch on Windows
        if(UNIX)
            add_custom_target(report
                COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_error_report.sh
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Running error detection and generating report"
            )
        else()
            add_custom_target(report
                COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_error_report.bat
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Running error detection and generating report"
            )
        endif()
    endif()
endif()

# ----------------------------------------------------------------------
# Coverage Configuration
# ----------------------------------------------------------------------
option(QUANTUMVERSE_USE_COVERAGE "Enable code coverage" OFF)
option(QUANTUMVERSE_ENABLE_TRACING "Enable performance tracing instrumentation" OFF)
option(QUANTUMVERSE_COVERAGE_MIN_LINE "Minimum line coverage threshold (percentage)" 95)
option(QUANTUMVERSE_COVERAGE_MIN_BRANCH "Minimum branch coverage threshold (percentage)" 90)

# Coverage thresholds
set(QUANTUMVERSE_COVERAGE_MIN_LINE ${QUANTUMVERSE_COVERAGE_MIN_LINE} CACHE STRING "Minimum line coverage threshold")
set(QUANTUMVERSE_COVERAGE_MIN_BRANCH ${QUANTUMVERSE_COVERAGE_MIN_BRANCH} CACHE STRING "Minimum branch coverage threshold")

if(QUANTUMVERSE_USE_COVERAGE)
    if(MSVC)
        # Windows: Use OpenCppCoverage (requires separate installation)
        message(STATUS "Coverage enabled for Windows - using OpenCppCoverage")
        message(STATUS "Note: OpenCppCoverage must be installed separately for coverage on Windows")
    else()
        # Linux/macOS: Use lcov/gcov
        message(STATUS "Coverage enabled for Unix - using lcov/gcov")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

if(QUANTUMVERSE_ENABLE_TRACING)
    message(STATUS "Performance tracing enabled")
    add_compile_definitions(QUANTUMVERSE_ENABLE_TRACING)
endif()

# Coverage report target
if(QUANTUMVERSE_USE_COVERAGE AND NOT MSVC)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage_report
            COMMAND ${LCOV_PATH} --capture --directory . --directory . --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '*/third_party/*' --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '*/tests/*' --output-file coverage.info
            COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage_report
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report"
        )
        
        # Coverage threshold check
        add_custom_target(coverage_check
            COMMAND ${LCOV_PATH} --capture --directory . --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '*/third_party/*' --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '*/tests/*' --output-file coverage.info
            COMMAND ${CMAKE_COMMAND} -D LCOV_INFO=coverage.info -D MIN_LINE=${QUANTUMVERSE_COVERAGE_MIN_LINE} -D MIN_BRANCH=${QUANTUMVERSE_COVERAGE_MIN_BRANCH} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CoverageCheck.cmake
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Checking coverage thresholds"
        )
    else()
        message(WARNING "lcov/genhtml not found - coverage report target not available")
    endif()
endif()

# ----------------------------------------------------------------------
# Performance Baseline Test
# ----------------------------------------------------------------------
add_executable(test_performance_baseline tests/test_performance_baseline.cpp)
target_link_libraries(test_performance_baseline PRIVATE dilaton)
add_test(NAME PerformanceBaselineTest COMMAND test_performance_baseline)

# ----------------------------------------------------------------------
# Stress/Extreme Tests
# ----------------------------------------------------------------------
add_executable(test_stress_extreme tests/test_stress_extreme.cpp)
target_link_libraries(test_stress_extreme PRIVATE dilaton)
add_test(NAME StressExtremeTest COMMAND test_stress_extreme)

# ----------------------------------------------------------------------
# Snapshot Tests
# ----------------------------------------------------------------------
add_executable(test_snapshot tests/test_snapshot.cpp)
target_link_libraries(test_snapshot PRIVATE dilaton)
add_test(NAME SnapshotTest COMMAND test_snapshot)

# ----------------------------------------------------------------------
# Time-Travel Debugging Support (rr on Linux, WinDbg TTD on Windows)
# ----------------------------------------------------------------------
option(QUANTUMVERSE_RR_RECORD "Enable rr time-travel debugging support" OFF)

if(QUANTUMVERSE_RR_RECORD AND NOT MSVC)
    message(STATUS "Time-travel debugging (rr) enabled")
    add_compile_definitions(QUANTUMVERSE_RR_RECORD)
endif()

if(QUANTUMVERSE_RR_RECORD AND MSVC)
    message(STATUS "WinDbg TTD time-travel debugging enabled")
    add_compile_definitions(QUANTUMVERSE_RR_RECORD)
endif()

# ----------------------------------------------------------------------
# Packaging Configuration (CPack)
# ----------------------------------------------------------------------
include(CPack)

set(CPACK_PACKAGE_NAME "QuantumVerse")
set(CPACK_PACKAGE_VERSION "0.1.1")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "QuantumVerse Simulator - 4D Spacetime Physics Engine")
set(CPACK_PACKAGE_VENDOR "QuantumVerse Team")
set(CPACK_PACKAGE_CONTACT "quantumverse@example.com")

# Install the main library and executables
install(TARGETS dilaton
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install Qt QML executable if built
if(QUANTUMVERSE_USE_QML AND Qt6_FOUND)
    install(TARGETS quantumverse_qml
        RUNTIME DESTINATION bin
    )
endif()

# Install ImGui executable if built
if(QUANTUMVERSE_USE_IMGUI)
    install(TARGETS quantumverse_imgui
        RUNTIME DESTINATION bin
    )
endif()

# Install test executables
install(TARGETS test_dilaton test_dilaton_blackhole test_hawking_calculator
    RUNTIME DESTINATION bin
)

# Install header files
install(DIRECTORY src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Windows packaging
if(WIN32)
    set(CPACK_GENERATOR "ZIP" "NSIS")
    set(CPACK_NSIS_INSTALL_ROOT "C:\\Program Files\\QuantumVerse")
    set(CPACK_NSIS_PACKAGE_NAME "QuantumVerse-${CPACK_PACKAGE_VERSION}-win64")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "
        CreateShortCut '\$SMPROGRAMS\\$STARTMENU_FOLDER\\QuantumVerse.lnk' '\$INSTDIR\\bin\\quantumverse_qml.exe'
    ")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "
        Delete '\$SMPROGRAMS\\$START_MENU\\QuantumVerse.lnk'
    ")
endif()

# Linux packaging
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ" "DEB" "RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgl1, libglu1-mesa")
endif()

# macOS packaging
if(APPLE)
    set(CPACK_GENERATOR "TGZ" "DragNDrop")
endif()
