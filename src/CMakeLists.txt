ADD_EXECUTABLE(tilda callback_func.c configsys.c eggaccelerators.c key_grabber.c tilda.c tilda_terminal.c tilda_window.c tomboykeybinder.c wizard.c xerror.c)
TARGET_LINK_LIBRARIES(tilda ${GLIB2_LIBRARIES} ${GTK3_LIBRARIES} ${VTE_LIBRARIES} ${CONFUSE_LIB})
INSTALL(TARGETS tilda DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
