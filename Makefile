EMCC=emcc
EMAR=emar

GENSOURCE=libdoom/m_config.c libdoom/i_sound.c libdoom/i_system.c libdoom/i_video.c libdoom/i_videohr.c libdoom/m_argv.c libdoom/m_misc.c libdoom/aes_prng.c libdoom/d_event.c libdoom/d_iwad.c libdoom/d_loop.c libdoom/d_mode.c libdoom/deh_str.c libdoom/i_endoom.c libdoom/i_glob.c libdoom/i_input.c libdoom/i_timer.c libdoom/m_bbox.c libdoom/m_cheat.c libdoom/m_controls.c libdoom/m_fixed.c libdoom/net_client.c libdoom/net_common.c libdoom/net_dedicated.c libdoom/net_io.c libdoom/net_loop.c libdoom/net_packet.c libdoom/net_petname.c libdoom/net_query.c libdoom/net_server.c libdoom/net_structrw.c libdoom/sha1.c libdoom/memio.c libdoom/tables.c libdoom/v_video.c libdoom/w_checksum.c libdoom/w_main.c libdoom/w_wad.c libdoom/w_file.c libdoom/w_file_stdc.c libdoom/w_file_posix.c libdoom/w_file_win32.c libdoom/w_merge.c libdoom/z_zone.c libdoom/deh_main.c libdoom/deh_io.c libdoom/deh_mapping.c libdoom/deh_text.c libdoom/quit.c libdoom/palette.c libdoom/main.c

GENOBJS=$(GENSOURCE:%.c=%.wasm)

TEXTSOURCE=libdoom/txt_desktop.c libdoom/txt_gui.c libdoom/txt_io.c libdoom/txt_label.c libdoom/txt_sdl.c libdoom/txt_separator.c libdoom/txt_strut.c libdoom/txt_table.c libdoom/txt_utf8.c libdoom/txt_widget.c libdoom/txt_window.c libdoom/txt_window_action.c

TEXTOBJS=$(TEXTSOURCE:%.c=%.wasm)

LIBOBJS=$(GENOBJS) $(TEXTOBJS)
CFLAGS=-O2 -I. -I./libdoom

DOOM=doom
DOOMSOURCE=doom/am_map.c doom/d_items.c doom/d_main.c doom/d_net.c doom/doomdef.c doom/doomstat.c doom/dstrings.c doom/f_finale.c doom/f_wipe.c doom/g_game.c doom/hu_lib.c doom/hu_stuff.c doom/info.c doom/m_menu.c doom/m_random.c doom/p_ceilng.c doom/p_doors.c doom/p_enemy.c doom/p_floor.c doom/p_inter.c doom/p_lights.c doom/p_map.c doom/p_maputl.c doom/p_mobj.c doom/p_plats.c doom/p_pspr.c doom/p_saveg.c doom/p_setup.c doom/p_sight.c doom/p_spec.c doom/p_switch.c doom/p_telept.c doom/p_tick.c doom/p_user.c doom/r_bsp.c doom/r_data.c doom/r_draw.c doom/r_main.c doom/r_plane.c doom/r_segs.c doom/r_sky.c doom/r_things.c doom/s_sound.c doom/sounds.c doom/st_lib.c doom/st_stuff.c doom/wi_stuff.c doom/deh_ammo.c doom/deh_bexstr.c doom/deh_cheat.c doom/deh_doom.c doom/deh_frame.c doom/deh_misc.c doom/deh_ptr.c doom/deh_sound.c doom/deh_thing.c doom/deh_weapon.c doom/doomgame.c
DOOMOBJS=$(DOOMSOURCE:%.c=%.wasm)
DOOMFLAGS=-I./doom -DDOOM

HERETIC=heretic
HERETICSOURCE=heretic/am_map.c heretic/d_main.c heretic/d_net.c heretic/deh_ammo.c heretic/deh_frame.c heretic/deh_htext.c heretic/deh_htic.c heretic/deh_sound.c heretic/deh_thing.c heretic/deh_weapon.c heretic/f_finale.c heretic/g_game.c heretic/in_lude.c heretic/info.c heretic/m_random.c heretic/mn_menu.c heretic/p_ceilng.c heretic/p_doors.c heretic/p_enemy.c heretic/p_floor.c heretic/p_inter.c heretic/p_lights.c heretic/p_map.c heretic/p_maputl.c heretic/p_mobj.c heretic/p_plats.c heretic/p_pspr.c heretic/p_saveg.c heretic/p_setup.c heretic/p_sight.c heretic/p_spec.c heretic/p_switch.c heretic/p_telept.c heretic/p_tick.c heretic/p_user.c heretic/r_bsp.c heretic/r_data.c heretic/r_draw.c heretic/r_main.c heretic/r_plane.c heretic/r_segs.c heretic/r_things.c heretic/s_sound.c heretic/sb_bar.c heretic/sounds.c heretic/hereticgame.c
HERETICOBJS=$(HERETICSOURCE:%.c=%.wasm)
HERETICFLAGS=-I./heretic -DHERETIC

HEXEN=hexen
HEXENSOURCE=hexen/a_action.c hexen/am_map.c hexen/d_net.c hexen/f_finale.c hexen/g_game.c hexen/h2_main.c hexen/in_lude.c hexen/info.c hexen/m_random.c hexen/mn_menu.c hexen/p_acs.c hexen/p_anim.c hexen/p_ceilng.c hexen/p_doors.c hexen/p_enemy.c hexen/p_floor.c hexen/p_inter.c hexen/p_lights.c hexen/p_map.c hexen/p_maputl.c hexen/p_mobj.c hexen/p_plats.c hexen/p_pspr.c hexen/p_setup.c hexen/p_sight.c hexen/p_spec.c hexen/p_switch.c hexen/p_telept.c hexen/p_things.c hexen/p_tick.c hexen/p_user.c hexen/po_man.c hexen/r_bsp.c hexen/r_data.c hexen/r_draw.c hexen/r_main.c hexen/r_plane.c hexen/r_segs.c hexen/r_things.c hexen/s_sound.c hexen/sb_bar.c hexen/sc_man.c hexen/sn_sonix.c hexen/sounds.c hexen/st_start.c hexen/sv_save.c hexen/hexengame.c
HEXENOBJS=$(HEXENSOURCE:%.c=%.wasm)
HEXENFLAGS=-I./hexen -DHEXEN

STRIFE=strife
STRIFESOURCE=strife/am_map.c strife/d_items.c strife/d_main.c strife/d_net.c strife/deh_ammo.c strife/deh_cheat.c strife/deh_frame.c strife/deh_misc.c strife/deh_ptr.c strife/deh_sound.c strife/deh_strife.c strife/deh_thing.c strife/deh_weapon.c strife/doomdef.c strife/doomstat.c strife/dstrings.c strife/f_finale.c strife/f_wipe.c strife/g_game.c strife/hu_lib.c strife/hu_stuff.c strife/info.c strife/m_menu.c strife/m_random.c strife/m_saves.c strife/p_ceilng.c strife/p_dialog.c strife/p_doors.c strife/p_enemy.c strife/p_floor.c strife/p_inter.c strife/p_lights.c strife/p_map.c strife/p_maputl.c strife/p_mobj.c strife/p_plats.c strife/p_pspr.c strife/p_saveg.c strife/p_setup.c strife/p_sight.c strife/p_spec.c strife/p_switch.c strife/p_telept.c strife/p_tick.c strife/p_user.c strife/r_bsp.c strife/r_data.c strife/r_draw.c strife/r_main.c strife/r_plane.c strife/r_segs.c strife/r_sky.c strife/r_things.c strife/s_sound.c strife/sounds.c strife/st_lib.c strife/st_stuff.c strife/wi_stuff.c strife/strifegame.c
STRIFEOBJS=$(STRIFESOURCE:%.c=%.wasm)
STRIFEFLAGS=-I./strife -DSTRIFE

LINKFLAGS=--no-entry -s INITIAL_MEMORY=67108864 -s IMPORTED_MEMORY -s EXPORTED_FUNCTIONS=_DoomInit,_DoomStep,_DoomKey,_DoomWadName,_DoomWadAlloc -s ERROR_ON_UNDEFINED_SYMBOLS=0

all: web/$(DOOM).wasm web/$(HERETIC).wasm web/$(HEXEN).wasm web/$(STRIFE).wasm

web/$(DOOM).wasm: $(DOOMOBJS) libdoom/libdoom.a
	$(EMCC) -o $@ $(LINKFLAGS) $(DOOMOBJS) libdoom/libdoom.a

web/$(HERETIC).wasm: $(HERETICOBJS) libdoom/libdoom.a
	$(EMCC) -o $@ $(LINKFLAGS) $(HERETICOBJS) libdoom/libdoom.a

web/$(HEXEN).wasm: $(HEXENOBJS) libdoom/libdoom.a
	$(EMCC) -o $@ $(LINKFLAGS) $(HEXENOBJS) libdoom/libdoom.a

web/$(STRIFE).wasm: $(STRIFEOBJS) libdoom/libdoom.a
	$(EMCC) -o $@ $(LINKFLAGS) $(STRIFEOBJS) libdoom/libdoom.a

libdoom/libdoom.a: $(LIBOBJS)
	$(EMAR) cr $@ $(LIBOBJS)

%.wasm: %.c
	$(EMCC) $(CFLAGS) -c -o $@ $<

doom/%.wasm: doom/%.c
	$(EMCC) $(CFLAGS) $(DOOMFLAGS) -c -o $@ $<

heretic/%.wasm: heretic/%.c
	$(EMCC) $(CFLAGS) $(HERETICFLAGS) -c -o $@ $<

hexen/%.wasm: hexen/%.c
	$(EMCC) $(CFLAGS) $(HEXENFLAGS) -c -o $@ $<

strife/%.wasm: strife/%.c
	$(EMCC) $(CFLAGS) $(STRIFEFLAGS) -c -o $@ $<

clean:
	rm -f web/$(DOOM).wasm $(DOOMOBJS)
	rm -f web/$(HERETIC).wasm $(HERETICOBJS)
	rm -f web/$(HEXEN).wasm $(HEXENOBJS)
	rm -f web/$(STRIFE).wasm $(STRIFEOBJS)
	rm -f $(LIBOBJS) libdoom/libdoom.a
