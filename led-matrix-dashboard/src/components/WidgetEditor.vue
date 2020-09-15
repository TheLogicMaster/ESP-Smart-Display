<template>
  <md-card>
    <h2> {{ $widgetNames[widget.type] }}</h2>
    <md-field class="layer-field field">
      <label>Layer</label>
      <md-input :min="0" :max="99" type="number" v-model.number="widget.id"></md-input>
    </md-field>
    <div v-if="(widget.type !== 4 && widget.type !== 6 && widget.type !== 2 && widget.type !== 3 && widget.type !== 8)">
      <md-checkbox v-model="dynamic" @change="changeDynamic">Dynamic</md-checkbox>
      <md-field class="layer-field field" v-if="widget.frequency !== 0">
        <label>Interval(ms)</label>
        <md-input :min="1" type="number" v-model.number="widget.frequency"></md-input>
      </md-field>
    </div>
    <template v-if="widget.type === 0 || widget.type === 1">
      <md-field class="image-field field">
        <label class="label">Image</label>
        <md-select name="Image" v-model="widget.content" @input="changeImage">
          <md-option v-for="(data, image) in $store.state.imageData" v-bind:key="image" :value="image">{{ image }}
          </md-option>
        </md-select>
      </md-field>
      <template v-if="widget.content !== ''">
        <md-checkbox v-model="widget.background">Background</md-checkbox>
        <div>
          <div class="field">
            <label class="label">Image Offset</label>
            <input @input="constrainLength" class="" min="0" :max="$store.state.imageData[widget.content].length - 1"
                   type="number" v-model.number="widget.offset">
          </div>
          <div class="field" v-if="widget.frequency > 0">
            <label class="label">Image Length</label>
            <input class="" min="1" :max="$store.state.imageData[widget.content].length - widget.offset" type="number"
                   v-model.number="widget.length">
          </div>
        </div>
      </template>
    </template>
    <template v-else-if="widget.type === 5 || widget.type === 9 || widget.type === 10 || widget.type === 11">
      <md-field class="text-content-field inline">
        <label>API URL</label>
        <md-input v-model="widget.source" @change="changeText"></md-input>
      </md-field>
    </template>
    <template v-else-if="widget.type === 4 || widget.type === 6">
      <md-field class="text-content-field inline">
        <label>Text Content</label>
        <md-input :maxlength="$store.state.stats.width / (widget.type === 4 ? 4 : 6)" v-model="widget.content" @change="changeText"></md-input>
      </md-field>
    </template>
    <div v-if="widget.type !== 0 && widget.type !== 1 && widget.type !== 3">
      <div class="field">
        <label class="label">Widget Width</label>
        <input class="" :min="getWidgetMinimumSize(widget).width" :max="$store.state.stats.width" type="number"
               v-model.number="widget.width">
      </div>
      <div class="field">
        <label class="label">Widget Height</label>
        <input class="" :min="getWidgetMinimumSize(widget).height" :max="$store.state.stats.height" type="number"
               v-model.number="widget.height">
      </div>
    </div>
    <div v-else-if="widget.type === 3">
      <div class="field">
        <label class="label">Widget Diameter</label>
        <input class="" min="5" :max="$store.state.stats.width" type="number" v-model.number="widget.width"
               @input="changeRadius">
      </div>
    </div>
    <div>
      <div class="field">
        <label class="label">Widget X</label>
        <input class="" min="0" :max="$store.state.stats.width - 1" type="number" v-model.number="widget.xOff">
      </div>
      <div class="field">
        <label class="label">Widget Y</label>
        <input class="" min="0" :max="$store.state.stats.height - 1" type="number" v-model.number="widget.yOff">
      </div>
    </div>
    <md-checkbox v-if="widget.type !== 0 && widget.type !== 1" v-model="widget.transparent">Transparent</md-checkbox>
    <md-checkbox v-model="widget.bordered">Bordered</md-checkbox>
    <div>
      <div class="field" v-for="(color, index) in widget.colors">
        <color-picker :label="requiredColors(widget.type)[index]" class="color-picker" v-model="widget.colors[index]"></color-picker>
      </div>
      <div class="field" v-if="!widget.transparent && widget.type !== 0 && widget.type !== 1">
        <color-picker label="Background" class="color-picker" v-model="widget.backgroundColor"></color-picker>
      </div>
      <div class="field" v-if="widget.bordered">
        <color-picker label="Border" class="color-picker" v-model="widget.borderColor"></color-picker>
      </div>
    </div>
    <md-checkbox v-model="widget.disabled">Disabled</md-checkbox>
    <md-button @click="$emit('delete')" class="md-accent md-raised">Delete</md-button>
    <md-button @click="$emit('clone')" class="md-accent md-raised">Clone</md-button>
  </md-card>
</template>

<script>
import ColorPicker from "@/components/ColorPicker";

export default {
  name: "WidgetEditor",
  components: {ColorPicker},
  methods: {
    changeText () {
      if (this.widget.type === 4)
        this.widget.content = this.widget.content.toUpperCase()
    },
    changeBackgroundColor(color) {
      this.widget.backgroundColor = this.jsHexToCpp(color)
    },
    changeBorderColor(color) {
      this.widget.borderColor = this.jsHexToCpp(color.hex)
    },
    changeRadius() {
      this.widget.height = this.widget.width
    },
    constrainLength() {
      this.widget.length = Math.min(this.$store.state.imageData[this.widget.content].length - this.widget.offset, this.widget.length)
    },
    changeDynamic() {
      if (!this.dynamic)
        this.widget.frequency = 0
      else if (this.widget.frequency === 0)
        this.widget.frequency = 1000
    },
    changeImage() {
      this.widget.offset = Math.min(this.widget.length - 1, this.widget.offset)
      this.constrainLength()
      this.widget.width = this.$store.state.imageData[this.widget.content].width
      this.widget.height = this.$store.state.imageData[this.widget.content].height
      this.widget.type = this.$store.state.imageData[this.widget.content].progmem ? 0 : 1
    },
    requiredColors() {
      if (this.widget.type === 0 || this.widget.type === 1 || this.widget.type === 7)
        return []
      else if (this.widget.type === 3)
        return ['Minute', 'Hour', 'Marking']
      else
        return ['Text']
    }
  },
  mounted() {
    this.dynamic = this.widget.frequency !== 0
  },
  data: () => ({
    dynamic: false
  }),
  props: ['widget']
}
</script>

<style scoped>
.color-picker {
  width: 100px;
}

.layer-field {
  width: 55px;
}

.image-field {
  width: 170px;
}

.text-content-field {
  width: 150px;
}

.label {
  margin-right: 5px;
}

.inline {
  display: inline-block;
}

.field {
  display: inline-block;
  margin: 3px;
}

.interval-field {
  width: 70px;
}
</style>
