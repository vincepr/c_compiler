@tool
extends "res://addons/graphtastic/dialog_player/normal_mode/speaker/gt_speaker_textbox.gd"
@export var singleton : NodePath

func set_GTD():
	GTD=get_node(singleton)
