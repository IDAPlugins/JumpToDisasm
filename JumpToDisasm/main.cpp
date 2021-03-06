#include <windows.h>
#include <tchar.h>

#include <hexrays.hpp>

#include <functional>

extern plugin_t PLUGIN;

// Hex-Rays API pointer
hexdsp_t *hexdsp = NULL;

namespace
{
	static bool inited = false;

	// Hotkey for the new command
	static const char hotkey_gd[] = "J";
}

// show disassembly line for ctree->item
bool idaapi decompiled_line_to_disasm_cb(void *ud)
{
	vdui_t &vu = *(vdui_t *)ud;
	vu.ctree_to_disasm();

	vu.get_current_item(USE_KEYBOARD);
	citem_t *highlight = vu.item.is_citem() ? vu.item.e : NULL;

	return true;
}

//--------------------------------------------------------------------------
// This callback handles various hexrays events.
static ssize_t idaapi callback(void *ud, hexrays_event_t event, va_list va)
{
	switch (event)
	{
	case hxe_populating_popup:
	{
		TWidget *widget = va_arg(va, TWidget *);
		TPopupMenu *popup = va_arg(va, TPopupMenu *);
		vdui_t &vu = *va_arg(va, vdui_t *);

		// add new command to the popup menu
		attach_action_to_popup(vu.ct, popup, "codexplorer::jump_to_disasm");
	}
	break;

	default:
		break;
	}
	return 0;
}

namespace
{
	class MenuActionHandler : public action_handler_t
	{
	public:
		typedef std::function<bool(void *)> handler_t;

		MenuActionHandler(handler_t handler)
			: handler_(handler)
		{
		}
		virtual int idaapi activate(action_activation_ctx_t *ctx)
		{
			auto vdui = get_widget_vdui(ctx->widget);
			return handler_(vdui) ? TRUE : FALSE;
		}

		virtual action_state_t idaapi update(action_update_ctx_t *ctx)
		{
			return ctx->widget_type == BWN_PSEUDOCODE ? AST_ENABLE_FOR_WIDGET : AST_DISABLE_FOR_WIDGET;
		}

	private:
		handler_t handler_;
	};

	static MenuActionHandler kJumpToDisasmHandler{ decompiled_line_to_disasm_cb };

	static action_desc_t kActionDescs[] = {
		ACTION_DESC_LITERAL("codexplorer::jump_to_disasm", "Jump to Disasm", &kJumpToDisasmHandler, hotkey_gd, nullptr, -1),
	};
}

//--------------------------------------------------------------------------
// Initialize the plugin.
int idaapi init()
{
	if (!init_hexrays_plugin())
		return PLUGIN_SKIP; // no decompiler

	for (unsigned i = 0; i < _countof(kActionDescs); ++i)
		register_action(kActionDescs[i]);

	install_hexrays_callback((hexrays_cb_t *)callback, nullptr);
	inited = true;

	return PLUGIN_KEEP;
}

//--------------------------------------------------------------------------
void idaapi term()
{
	if (inited)
	{
		remove_hexrays_callback((hexrays_cb_t *)callback, NULL);
		term_hexrays_plugin();
	}
}

//--------------------------------------------------------------------------
bool idaapi run(size_t)
{
	// This function won't be called because our plugin is invisible (no menu
	// item in the Edit, Plugins menu) because of PLUGIN_HIDE
	return true;
}

static const char comment[] = "JumpToDisasm plugin";

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
	IDP_INTERFACE_VERSION,
	PLUGIN_HIDE,			// plugin flags
	init,					// initialize
	term,					// terminate. this pointer may be NULL.
	run,					// invoke plugin
	comment,				// long comment about the plugin
							// it could appear in the status line or as a hint
	"",						// multiline help about the plugin
	"JumpToDisasm",			// the preferred short name of the plugin (PLUGIN.wanted_name)
	""						// the preferred hotkey to run the plugin
};