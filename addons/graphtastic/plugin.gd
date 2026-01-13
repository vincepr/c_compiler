@tool
extends EditorPlugin

const MainPanel = preload("res://addons/graphtastic/dialog_editor/starter_panel.tscn")
var main_panel_instance

# names used to register and cleanup addon elements
const NAME_AUTOLOAD = "GTD"
const NAME_DIALOG_TEXTBOX = "GTDialog_Textbox"
const NAME_DIALOG_CHOICES = "GTDialog_Choices"
const NAME_DIALOG_PICTURE = "GTDialog_Picture"
const NAME_DIALOG_SPEAKER = "GTDialog_SpeakerLabel"

func _enter_tree():
	#The Singleton:	
	add_autoload_singleton (NAME_AUTOLOAD, "res://addons/graphtastic/helper_classes/gt_variablesdata_singleton.gd")
	
	#add node-types:
	add_custom_type(NAME_DIALOG_TEXTBOX, "RichTextLabel", preload("dialog_player/normal_mode/textbox/gt_dialog_textbox.gd"), preload("graphtastic_icon.png"))
	add_custom_type(NAME_DIALOG_CHOICES, "VBoxContainer", preload("dialog_player/normal_mode/choices/gt_vbox_choices.gd"), preload("graphtastic_icon.png"))
	add_custom_type(NAME_DIALOG_PICTURE, "TextureRect", preload("dialog_player/normal_mode/pictures/gt_texture_rect.gd"), preload("graphtastic_icon.png"))
	add_custom_type(NAME_DIALOG_SPEAKER, "RichTextLabel", preload("dialog_player/normal_mode/speaker/gt_speaker_textbox.gd"), preload("graphtastic_icon.png"))
	
	#Editor in MainPanel:
	main_panel_instance = MainPanel.instantiate()
	get_editor_interface().get_editor_main_screen().add_child(main_panel_instance)
	## TODO: get undo redo to work!
	#main_panel_instance.undo_redo = get_undo_redo()
	## TODO: do we still need _make_visible(false) what even was the purpose?
	#_make_visible(false)
	
	#add_custom_type("GraphtasticPlayer", "Panel?", preload(...DialogueBoxScene), preload(...icon))

func _exit_tree():
	#remove_custom_type("GraphtasticPlayer")
	
	remove_autoload_singleton(NAME_AUTOLOAD)
	remove_custom_type(NAME_DIALOG_TEXTBOX)
	remove_custom_type(NAME_DIALOG_CHOICES)
	remove_custom_type(NAME_DIALOG_PICTURE)
	remove_custom_type(NAME_DIALOG_SPEAKER)
	
	# check if this is still necessary
	if is_instance_valid(main_panel_instance):
		main_panel_instance.queue_free()
		



func has_main_screen():
	return true


func make_visible(visible):
	if visible:
		main_panel_instance.show()
	else:
		main_panel_instance.hide()


func get_plugin_name():
	return "GraphTasticDialog"


func get_plugin_icon():
	# Must return some kind of Texture for the icon.
	return preload("graphtastic_icon.png")			#get_editor_interface().get_base_control().get_icon("Node", "EditorIcons")
