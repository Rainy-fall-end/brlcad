# manually sourcing dependencies for now
if {[catch {
    set script [file join [bu_brlcad_data "tclscripts"] boteditor botEditor.tcl]
    source $script
} errMsg] > 0} {
    puts "Couldn't load \"$script\"\n$errMsg"
    exit
}

proc bot_askforname {parent screen} {
    if {[cad_input_dialog $parent.botname $screen "BoT to Edit"\
	   "Enter name of BoT:" botname ""\
	   0 {{ summary "Object name of BoT to edit with BoT Editor"} { see_also bot_decimate }} OK Cancel] == 1} {
		   return
    }
    BotEditor .botedit $botname
}


