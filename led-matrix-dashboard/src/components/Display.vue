<template>
  <div>
    <div class="display-panel">
      <md-button @click="presetDialog = true" class="md-raised md-accent">Load Preset</md-button>
      <md-button @click="reload" class="md-raised md-accent">Reload Configuration</md-button>
      <md-button @click="save" class="md-raised md-accent">Save Configuration</md-button>
      <color-picker label="Background" class="color-picker" v-model="config.backgroundColor"></color-picker>
      <md-content class="display"
                  v-bind:style="{width: `${64 * pixelSize}px`, height: `${32 * pixelSize}px`, backgroundColor: cppHexToJs(config.backgroundColor)}">
        <widget @doubletap="scrollToWidget(widget.id)" :pixel-size="pixelSize" v-for="widget in config.widgets"
                v-bind:key="createUniqueKey(widget)" :widget="widget"
                v-if="!widget.disabled"></widget>
      </md-content>
    </div>

    <md-content class="md-primary widget-panel">
      <h1>Widgets</h1>
      <div v-if="config.widgets.length === 0">
        Create a Widget to get started
      </div>
      <md-button @click="addWidgetDialog = true" class="md-raised md-accent">Add Widget</md-button>
      <md-card id="widgets" v-bind:style="{'max-height': $isMobile() ? null : '490px' }"
               class="md-accent md-scrollbar widget-panel-widgets">
        <widget-editor :id="`widget${widget.id}`" @delete="config.widgets.splice(index, 1)"
                       @clone="config.widgets.push(cloneObject(widget))" class="widget-editor"
                       v-for="(widget, index) in config.widgets" v-bind:key="createUniqueKey(widget)"
                       :widget="widget"></widget-editor>
      </md-card>
    </md-content>

    <md-dialog :md-active.sync="addWidgetDialog">
      <md-dialog-title>Create Widget</md-dialog-title>
      <md-field>
        <md-select v-model="addWidgetType">
          <md-option v-for="(name, key) in this.$widgetNames" :key="key" :value="key"> {{ name }}</md-option>
        </md-select>
      </md-field>
      <md-dialog-actions>
        <md-button class="md-accent md-raised" @click="addWidgetDialog = false">Cancel</md-button>
        <md-button class="md-accent md-raised" @click="createWidget">Create Widget</md-button>
      </md-dialog-actions>
    </md-dialog>

    <md-dialog :md-active.sync="presetDialog">
      <md-dialog-title>Load Preset</md-dialog-title>
      <md-field>
        <md-select v-model="presetName">
          <md-option v-for="(preset, name) in presets" :key="name" :value="name"> {{ name }}</md-option>
        </md-select>
      </md-field>
      <md-dialog-actions>
        <md-button class="md-accent md-raised" @click="presetDialog = false">Cancel</md-button>
        <md-button class="md-accent md-raised" @click="loadPreset">Create Widget</md-button>
      </md-dialog-actions>
    </md-dialog>
  </div>
</template>

<script>
import Widget from "@/components/Widget";
import WidgetEditor from "@/components/WidgetEditor";
import ColorPicker from "@/components/ColorPicker";

export default {

  name: "Display",
  components: {ColorPicker, WidgetEditor, Widget},
  data: () => ({
    config: {
      widgets: [],
      backgroundColor: '#000000'
    },
    grid: null,
    pixelSize: 10,
    addWidgetDialog: false,
    addWidgetType: 0,
    presetDialog: false,
    presetName: 'Black Lives Matter',
    presets: {
      'Black Lives Matter': [
        {
          id: 0,
          type: 0,
          xOff: 0,
          yOff: 0,
          width: 64,
          height: 32,
          offset: 0,
          content: 'blm',
          disabled: false,
          background: true,
          frequency: 5000,
          length: 2
        }
      ]
    }
  }),
  methods: {
    loadPreset() {
      this.config.widgets.length = 0
      for (let i in this.presets[this.presetName])
        this.config.widgets.push(this.bloatWidget(this.presets[this.presetName][i]))
      this.presetDialog = false
    },
    scrollToWidget(id) {
      this.$scrollTo(`#widget${id}`, 500, {
        container: '#widgets',
        easing: 'ease-in',
        offset: -60,
        force: true,
        cancelable: true,
        x: false,
        y: true
      })
    },
    createWidget() {
      this.addWidgetDialog = false

      let widget = this.createDefaultWidget(this.addWidgetType)
      let id = 0
      for (let index in this.config.widgets)
        id = Math.max(id, this.config.widgets[index].id + 1)
      widget.id = id
      this.config.widgets.push(widget)
      this.addWidgetType = 0
    },
    async reload() {
      let result = await this.getConfig()
      if (result) {
        this.setup()
      }
    },
    setup() {
      this.config = this.cloneObject(this.$store.state.configuration)
    },
    async save() {
      let result = await this.saveConfig(this.config)
    }
  },
  async mounted() {
    await this.waitForConfiguration()
    await this.waitForStats()
    await this.getImageData()
    this.refreshImages(false).then()
    this.setup()
  }
}
</script>

<style scoped>
.display {
  position: relative;
}

.display-panel {
  display: inline-block;
  vertical-align: top;
}

.widget-panel {
  width: 400px;
  display: inline-block;
}

.widget-panel-widgets {
  overflow: auto;
}

.widget-editor {
  margin-top: 5px;
  margin-bottom: 5px;
  overflow-x: hidden
}
</style>
