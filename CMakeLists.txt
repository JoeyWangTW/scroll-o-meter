# ZMK app headers (zmk/event_manager.h, zmk/events/*, zmk/behavior.h,
# zmk/behavior_queue.h) live at app/include but aren't on the default
# include path for external Zephyr modules. CMAKE_SOURCE_DIR points at
# zmk/app when ZMK is the top-level Zephyr application.
set(ZMK_APP_INCLUDE "${CMAKE_SOURCE_DIR}/include")

if(CONFIG_ZMK_AGENT_IOS_POINTER_NUDGE)
  zephyr_library_named(scroll_meter_ios_pointer_nudge)
  zephyr_library_sources(src/ios_pointer_nudge.c)
endif()

if(CONFIG_ZMK_SCROLL_COUNTER)
  zephyr_library_named(scroll_meter_counter)
  zephyr_library_sources(src/scroll_counter.c)
  zephyr_library_sources(src/behavior_count_print.c)
  zephyr_library_sources(src/behavior_count_reset.c)
  zephyr_library_include_directories(
    ${ZMK_APP_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
endif()
