#pragma once

#include <string>

namespace morph {
    namespace gl {
        // I want to encode the GL version in a single int. This struct provides the static methods
        // to decode.
        //
        // These are our version numbers:
        static constexpr int version_4_1        = 0x00040001;
        static constexpr int version_4_1_compat = 0x20040001;
        static constexpr int version_4_2        = 0x00040002;
        static constexpr int version_4_2_compat = 0x20040002;
        static constexpr int version_4_3        = 0x00040003;
        static constexpr int version_4_3_compat = 0x20040003;
        static constexpr int version_4_4        = 0x00040004;
        static constexpr int version_4_4_compat = 0x20040004;
        static constexpr int version_4_5        = 0x00040005;
        static constexpr int version_4_5_compat = 0x20040005;
        static constexpr int version_4_6        = 0x00040006;
        static constexpr int version_4_6_compat = 0x20040006;
        static constexpr int version_3_0_es     = 0x40030000; // OpenGL 3.0 ES is a subset of OpenGL 3.3
        static constexpr int version_3_1_es     = 0x40030001; // OpenGL 3.1 ES is a subset of OpenGL 4.3
        static constexpr int version_3_2_es     = 0x40030002;
        struct version
        {
            // Open GL minor version number (note: outdated versions with a 3rd number such as 1.2.1 are NOT supported)
            static int constexpr minor (const int gl_version_number) { return (gl_version_number & 0xffff); }
            // Open GL major version number
            static int constexpr major (const int gl_version_number) { return (gl_version_number >> 16 & 0x1fff); }
            // True if this is the compatibility profile (by default it's the core profile)
            static bool constexpr compat (const int gl_version_number) { return (((gl_version_number >> 29) & 0x1) > 0x0) ? true : false; }
            // True if this is an OpenGL ES version
            static bool constexpr gles (const int gl_version_number) { return (((gl_version_number >> 30) & 0x1) > 0x0) ? true : false; }
            // Output a string describing the version number
            static std::string vstring (const int gl_version_number)
            {
                std::string v = std::to_string (version::major(gl_version_number)) + std::string(".")
                + std::to_string (version::minor(gl_version_number));
                if (version::compat(gl_version_number)) {
                    v += " compat";
                }
                if (version::gles(gl_version_number)) {
                    v += " ES";
                }
                return v;
            }
            // Output the shader version string
            static std::string shaderversion (const int gl_version_number)
            {
                std::string v("#version ");
                v += std::to_string (version::major(gl_version_number))
                + std::to_string (version::minor(gl_version_number)) + std::string("0");
                if (version::gles(gl_version_number)) {
                    v += " es";
                }
                return v;
            }
            // Return the version-specific shader preamble in a constexpr function
            static constexpr const char* shaderpreamble (const int gl_version_number)
            {
                const char* preamble = "#version unknown\n";

                switch (gl_version_number) {
                case morph::gl::version_3_0_es:
                    preamble = "#version 300 es\n#extension GL_EXT_shader_io_blocks : enable\nprecision mediump float;\n";
                    break;
                case morph::gl::version_3_1_es:
                    preamble = "#version 310 es\n#extension GL_EXT_shader_io_blocks : enable\nprecision mediump float;\n";
                    break;
                case morph::gl::version_3_2_es:
                    preamble = "#version 320 es\n#extension GL_EXT_shader_io_blocks : enable\nprecision mediump float;\n";
                    break;
                case morph::gl::version_4_1:
                case morph::gl::version_4_1_compat:
                    preamble = "#version 410\n";
                    break;
                case morph::gl::version_4_2:
                case morph::gl::version_4_2_compat:
                    preamble = "#version 420\n";
                    break;
                case morph::gl::version_4_3:
                case morph::gl::version_4_3_compat:
                    preamble = "#version 430\n";
                    break;
                case morph::gl::version_4_4:
                case morph::gl::version_4_4_compat:
                    preamble = "#version 440\n";
                    break;
                case morph::gl::version_4_5:
                case morph::gl::version_4_5_compat:
                    preamble = "#version 450\n";
                    break;
                case morph::gl::version_4_6:
                case morph::gl::version_4_6_compat:
                    preamble = "#version 460\n";
                    break;
                default:
                    break;
                }
                return preamble;
            }
        };
    }
}
