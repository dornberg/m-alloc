function(malloc_set_warnings target warnings_as_errors)
    if(MSVC)
        set(warnings /W4 /permissive-)
        if(warnings_as_errors)
            list(APPEND warnings /WX)
        endif()
    else()
        set(warnings
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wnull-dereference
            -Wdouble-promotion)
        if(warnings_as_errors)
            list(APPEND warnings -Werror)
        endif()
    endif()
    target_compile_options(${target} PRIVATE ${warnings})
endfunction()
