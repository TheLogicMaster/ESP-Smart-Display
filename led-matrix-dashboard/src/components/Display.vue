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
                       :widget="widget"
                       @reorder="config.widgets.sort((a, b) => (a.id >= b.id) ? 1 : -1)"></widget-editor>
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
        <md-select v-model="presetName" @input="parameters = []">
          <md-option v-for="(args, name) in presets" :key="name" :value="name"> {{ name }}</md-option>
        </md-select>
      </md-field>
      <md-field v-for="(label, key) in presets[presetName]">
        <label>{{ label }}</label>
        <md-input v-model="parameters[key]"></md-input>
      </md-field>
      <md-dialog-actions>
        <md-button class="md-accent md-raised" @click="presetDialog = false">Cancel</md-button>
        <md-button class="md-accent md-raised" @click="loadPreset">Load Preset</md-button>
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
    parameters: [],
    grid: null,
    pixelSize: 10,
    addWidgetDialog: false,
    addWidgetType: 0,
    presetDialog: false,
    presetName: 'Black Lives Matter',
    presets: {
      'Black Lives Matter': [],
      'Mario Clock': [],
      'Youtube Subscriber Count': ['Channel ID', 'Google API Key']
    }
  }),
  methods: {
    loadPreset() {
      this.config.widgets.length = 0

      let widgets = []
      switch (this.presetName) {
        case "Black Lives Matter":
          widgets = [
            {
              width: 64,
              height: 32,
              content: 'blm',
              background: true,
              frequency: 5000,
              length: 2
            }
          ]
          break
        case 'Mario Clock':
          widgets = [
            {
              width: 64,
              height: 32,
              content: 'mario',
              background: true,
              length: 1
            },
            {
              id: 1,
              type: 2,
              large: true,
              xOff: 31,
              yOff: 23,
              width: 32,
              height: 9,
              colors: ['0x000000'],
              transparent: true
            }
          ]
          break
        case 'Youtube Subscriber Count':
          widgets = [
            {
              "id": 2, // UCFKDEp9si4RmHFWJW1vYsMA
              "type": 5,
              "source": `https://www.googleapis.com/youtube/v3/channels?part=statistics&id=${this.parameters[0]}&key=${this.parameters[1]}`,
              "length": 15000,
              "frequency": 60000,
              "borderColor": '0xFF0000',
              "backgroundColor": '0x00FFFF',
              "args": ['items', '0', 'statistics', 'subscriberCount'],
              "colors": ['0x00FF00'],
              "width": 29,
              "height": 5
            }
          ]
          break
      }

      for (let i in widgets)
        this.config.widgets.push(this.bloatWidget(widgets[i]))
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
