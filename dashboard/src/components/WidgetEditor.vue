<template>
  <md-card>
    <h2 class="inline"> {{ $widgetNames[widget.type] }}</h2>
    <div class="md-alignment-space-between">
      <div class="inline-centered">
        <md-checkbox v-model="collapsed">Collapsed</md-checkbox>
        <md-tooltip md-delay="1000" md-direction="left"> Collapse the widget </md-tooltip>
      </div>
      <div class="inline-centered">
        <md-field class="layer-field">
          <label>Layer</label>
          <md-input :min="0" :max="99" type="number" v-model.number="widget.id" @change="$emit('reorder')"></md-input>
          <md-tooltip md-delay="1000" md-direction="left"> Change the layer the widget sits on</md-tooltip>
        </md-field>
      </div>
    </div>
    <template v-if="!collapsed">
      <div v-if="(widget.type !== 4 && widget.type !== 2 && widget.type !== 3 && widget.type !== 8)">
        <div class="inline-centered">
          <md-checkbox v-model="dynamic" @change="changeDynamic">Dynamic</md-checkbox>
          <md-tooltip md-delay="1000" md-direction="left"> Enable dynamic widget content </md-tooltip>
        </div>
        <div class="inline-centered">
          <md-field class="layer-field" v-if="dynamic">
            <label>Interval(ms)</label>
            <md-input :min="1" type="number" v-model.number="widget.frequency"></md-input>
            <md-tooltip md-delay="1000" md-direction="left"> Change the widget update frequency </md-tooltip>
          </md-field>
        </div>
      </div>
      <template v-if="widget.type === 0 || widget.type === 1">
        <div class="inline-centered">
          <md-field class="image-field field">
            <label class="label">Image</label>
            <md-select name="Image" v-model="widget.content" @input="changeImage">
              <md-option v-for="(data, image) in $store.state.imageData" v-bind:key="image" :value="image">{{ image }}
              </md-option>
            </md-select>
            <md-tooltip md-delay="1000" md-direction="left"> Change the widget image </md-tooltip>
          </md-field>
        </div>
        <div class="inline-centered">
          <md-field class="field" style="width: 80px">
            <label>Alpha Colors</label>
            <md-input :min="0" :max="10" type="number" v-model.number="alphaColors" @input="changeAlphaColors"></md-input>
            <md-tooltip md-delay="1000" md-direction="left"> Change the number of image colors to not render </md-tooltip>
          </md-field>
        </div>
        <div>
          <div v-for="(color, key) in widget.colors" :key="key" class="inline-centered">
            <color-picker  class="get-arg" v-model="widget.colors[key]" :label="'Alpha ' + (key + 1)"></color-picker>
            <md-tooltip md-delay="1000" md-direction="left"> Specify color to not render on image </md-tooltip>
          </div>
        </div>
        <template v-if="widget.content !== '' && $store.state.imageData[widget.content]">
          <div class="inline-centered">
            <md-checkbox v-model="widget.background">Background</md-checkbox>
            <md-tooltip md-delay="1000" md-direction="left"> If the widget exists in the background </md-tooltip>
          </div>
          <div>
            <div class="field">
              <label class="label">Image Offset</label>
              <input @input="constrainLength" class="" min="0" :max="$store.state.imageData[widget.content].length - 1"
                     type="number" v-model.number="widget.offset">
              <md-tooltip md-delay="1000" md-direction="left"> The first frame of the image to use </md-tooltip>
            </div>
            <div class="field" v-if="widget.frequency > 0">
              <label class="label">Image Length</label>
              <input class="" min="1" :max="$store.state.imageData[widget.content].length - widget.offset" type="number"
                     v-model.number="widget.length">
              <md-tooltip md-delay="1000" md-direction="left"> The number of image frames to use </md-tooltip>
            </div>
          </div>
        </template>
      </template>
      <template v-else-if="widget.type === 5 || widget.type === 6">
        <div>
          <md-field class="text-content-field inline">
            <label>API URL</label>
            <md-input class="text-content-field" v-model="widget.source"></md-input>
            <md-tooltip md-delay="1000" md-direction="left"> The URL of the API to GET </md-tooltip>
          </md-field>
        </div>
        <div>
          <md-field class="text-content-field inline">
            <label>CA Certificate</label>
            <md-textarea md-autogrow class="text-content-field" v-model="widget.cert" @change="changeCert"></md-textarea>
            <md-tooltip md-delay="1000" md-direction="left"> The CA certificate to verify HTTPS connections </md-tooltip>
          </md-field>
        </div>
        <md-field class="duration-field field">
          <label>Timeout(ms)</label>
          <md-input class="duration-field" step="1000" min="0" type="number" v-model.number="widget.length"></md-input>
          <md-tooltip md-delay="1000" md-direction="left"> The timeout for the GET request </md-tooltip>
        </md-field>
        <template v-if="widget.type === 5">
          <md-field class="auth-select field">
            <label>Auth</label>
            <md-select :value="getAuthType(this.widget.auth)" @input="changeAuth" name="Auth">
              <md-option :value="0"> No Auth </md-option>
              <md-option :value="1"> Basic Auth </md-option>
              <md-option :value="2"> Token Auth</md-option>
            </md-select>
            <md-tooltip md-delay="1000" md-direction="left"> The type of auth for the request</md-tooltip>
          </md-field>
          <md-field class="layer-field field">
            <label>Args</label>
            <md-input :min="1" :max="10" type="number" v-model.number="args" @input="changeArgs"></md-input>
            <md-tooltip md-delay="1000" md-direction="left"> The number of JSON indices to parse response with </md-tooltip>
          </md-field>
          <div>
            <md-field v-for="(arg, key) in widget.args" :key="key" class="get-arg inline">
              <label>Arg {{key + 1}}</label>
              <md-input v-model="widget.args[key]"></md-input>
              <md-tooltip md-delay="1000" md-direction="left"> A JSON string or integer index </md-tooltip>
            </md-field>
          </div>
        </template>
        <md-field class="text-content-field inline" v-if="widget.auth.length !== 0">
          <label> {{ widget.auth.startsWith('Basic') ? 'username:password' : 'Token' }} </label>
          <md-input class="text-content-field" v-model="auth" @input="changeAuthContent"></md-input>
          <md-tooltip md-delay="1000" md-direction="left"> The auth value for the request </md-tooltip>
        </md-field>
      </template>
      <template v-else-if="widget.type === 4">
        <md-field class="text-content-field inline">
          <label>Text Content</label>
          <md-input :maxlength="$store.state.stats.width / getTextSize(1, widget.font).width" v-model="widget.content" @change="changeText"></md-input>
          <md-tooltip md-delay="1000" md-direction="left"> The text for the widget </md-tooltip>
        </md-field>
      </template>
      <template v-else-if="widget.type === 2">
        <md-field class="field">
          <label class="label">Clock Type</label>
          <md-select name="Clock Type" v-model="widget.contentType">
            <md-option :value="0"> 24 Hour </md-option>
            <md-option :value="1"> 12 Hour </md-option>
            <md-option :value="2"> 12 Hour with Period </md-option>
          </md-select>
          <md-tooltip md-delay="1000" md-direction="left"> The time format for the widget </md-tooltip>
        </md-field>
      </template>
      <template v-else-if="widget.type === 8">
        <md-field class="field">
          <label class="label">Shape</label>
          <md-select name="Shape" v-model="widget.contentType" @input="changeShape">
            <md-option :value="0"> Rectangle </md-option>
            <md-option :value="1"> Rounded Rectangle </md-option>
            <md-option :value="2"> Circle </md-option>
            <md-option :value="3"> Pixel </md-option>
          </md-select>
          <md-tooltip md-delay="1000" md-direction="left"> The shape for the widget </md-tooltip>
        </md-field>
      </template>
      <template v-else-if="widget.type === 9">
        <md-field class="variable-field field">
          <label class="label">Variable</label>
          <md-select name="Variable" v-model="widget.offset">
            <md-option :value="0"> Temperature </md-option>
            <md-option :value="1"> Perceived Temperature </md-option>
            <md-option :value="2"> Temperature Max </md-option>
            <md-option :value="3"> Temperature Min </md-option>
            <md-option :value="4"> Visibility </md-option>
            <md-option :value="5"> Barometric Pressure </md-option>
            <md-option :value="6"> Humidity </md-option>
            <md-option :value="7"> Wind Speed </md-option>
          </md-select>
          <md-tooltip md-delay="1000" md-direction="left"> The variable for the widget </md-tooltip>
        </md-field>
        <div class="field" v-if="widget.offset <= 7" >
          <md-checkbox v-model="widget.length">Units</md-checkbox>
        </div>
      </template>
      <template v-if="widget.type === 2 || widget.type === 4 || widget.type === 5 || widget.type === 6 || widget.type === 9">
        <md-field class="field" v-if="widget.type !== 2">
          <label class="label">Text Mode</label>
          <md-select name="Text Mode" v-model="widget.contentType">
            <md-option :value="0"> Don't Wrap Text </md-option>
            <md-option :value="1"> Wrap Every Character </md-option>
            <md-option :value="2"> Wrap Words </md-option>
          </md-select>
          <md-tooltip md-delay="1000" md-direction="left"> The text-wrapping for the widget </md-tooltip>
        </md-field>
        <md-field class="field">
          <label class="label">Font</label>
          <md-select name="Font" v-model="widget.font" @input="changeFont">
            <md-option :value="0"> Tiny </md-option>
            <md-option :value="1"> Tetris </md-option>
            <md-option :value="2"> Normal </md-option>
            <md-option :value="3"> Large </md-option>
            <md-option :value="4"> Huge </md-option>
          </md-select>
          <md-tooltip md-delay="1000" md-direction="left"> The font for the widget </md-tooltip>
        </md-field>
      </template>
      <div v-if="widget.type !== 0 && widget.type !== 1 && widget.type !== 3 && widget.type !== 8 || (widget.type === 8 && widget.contentType < 2)">
        <div class="field">
          <label class="label">Widget Width</label>
          <input class="" :min="getWidgetMinimumSize(widget).width" :max="$store.state.stats.width * 2" type="number"
                 v-model.number="widget.width">
          <md-tooltip md-delay="1000" md-direction="left"> The width of the widget </md-tooltip>
        </div>
        <div class="field">
          <label class="label">Widget Height</label>
          <input class="" :min="getWidgetMinimumSize(widget).height" :max="$store.state.stats.height * 2" type="number"
                 v-model.number="widget.height">
          <md-tooltip md-delay="1000" md-direction="left"> The height of the widget </md-tooltip>
        </div>
      </div>
      <div v-else-if="widget.type === 3 || (widget.type === 8 && widget.contentType === 2)">
        <div class="field">
          <label class="label">Widget Diameter</label>
          <input class="" min="5" :max="$store.state.stats.width" type="number" v-model.number="widget.width"
                 @input="changeRadius">
          <md-tooltip md-delay="1000" md-direction="left"> The diameter of the widget </md-tooltip>
        </div>
      </div>
      <div>
        <div class="field">
          <label class="label">Widget X</label>
          <input class="" min="0" :max="$store.state.stats.width - 1" type="number" v-model.number="widget.xOff">
          <md-tooltip md-delay="1000" md-direction="left"> The X offset for the widget </md-tooltip>
        </div>
        <div class="field">
          <label class="label">Widget Y</label>
          <input class="" min="0" :max="$store.state.stats.height - 1" type="number" v-model.number="widget.yOff">
          <md-tooltip md-delay="1000" md-direction="left"> The Y offset for the widget </md-tooltip>
        </div>
      </div>
      <div>
        <md-checkbox v-if="widget.type === 8 || widget.type === 4" v-model="widget.background" @change="changeBackground">Background</md-checkbox>
        <md-tooltip md-delay="1000" md-direction="left"> Weather the widget exists in the background </md-tooltip>
      </div>
      <div class="inline-centered">
        <md-checkbox v-if="(widget.contentType !== 3 || widget.type !== 8) && !widget.background" v-model="widget.transparent">Transparent</md-checkbox>
        <md-tooltip md-delay="1000" md-direction="left"> Enable widget transparency </md-tooltip>
      </div>
      <template v-if="widget.type !== 0 && widget.type !== 1">
        <div class="inline-centered">
          <md-checkbox v-if="widget.type !== 8" v-model="widget.bordered">Bordered</md-checkbox>
          <md-tooltip md-delay="1000" md-direction="left"> Enable widget border </md-tooltip>
        </div>
        <div class="field" v-for="(color, index) in widget.colors">
          <color-picker :label="requiredColors(widget.type)[index]" class="color-picker" v-model="widget.colors[index]"></color-picker>
          <md-tooltip md-delay="1000" md-direction="left"> Set widget color </md-tooltip>
        </div>
      </template>
      <div class="field" v-if="widget.type === 8 && widget.contentType === 3">
        <color-picker label="Pixel" class="color-picker" v-model="widget.backgroundColor"></color-picker>
        <md-tooltip md-delay="1000" md-direction="left"> The pixel color </md-tooltip>
      </div>
      <div class="field" v-else-if="!widget.transparent && widget.type !== 0 && widget.type !== 1">
        <color-picker label="Background" class="color-picker" v-model="widget.backgroundColor"></color-picker>
        <md-tooltip md-delay="1000" md-direction="left"> The background color for the widget </md-tooltip>
      </div>
      <div class="field" v-if="widget.bordered">
        <color-picker label="Border" class="color-picker" v-model="widget.borderColor"></color-picker>
        <md-tooltip md-delay="1000" md-direction="left"> The border color for the widget </md-tooltip>
      </div>
    </template>
    <div>
      <div class="inline-centered">
        <md-checkbox v-model="widget.disabled">Disabled</md-checkbox>
        <md-tooltip md-delay="1000" md-direction="left"> Disable the widget </md-tooltip>
      </div>
      <div class="inline-centered">
        <md-button @click="tryDelete" class="md-raised" :class="deleting ? 'md-primary' : 'md-accent'">Delete</md-button>
        <md-tooltip md-delay="1000" md-direction="left"> Delete the widget </md-tooltip>
      </div>
      <div class="inline-centered">
        <md-button @click="$emit('clone')" class="md-accent md-raised">Clone</md-button>
        <md-tooltip md-delay="1000" md-direction="left"> Clone the widget </md-tooltip>
      </div>
    </div>
  </md-card>
</template>

<script>
import ColorPicker from "@/components/ColorPicker";

export default {
  name: "WidgetEditor",
  components: {ColorPicker},
  computed: {
    getAuthContent: function() {
      if (this.widget.auth.startsWith('Basic '))
        return this.widget.auth.substr(7)
      else if (this.widget.auth.startsWith('Bearer '))
        return this.widget.auth.substr(8)
      else
        return ''
    }
  },
  methods: {
    async tryDelete() {
      if (this.deleting)
        this.$emit('delete')
      else {
        this.deleting = true
        await this.sleep(3000)
        this.deleting = false
      }
    },
    getAuthType(auth) {
      if (auth.startsWith('Basic '))
        return 1
      else if (auth.startsWith('Bearer '))
        return 2
      else
        return 0
    },
    changeFont(font) {
      if (font === 1)
        this.widget.frequency = 100
      else if (this.widget.type === 4)
        this.widget.frequency = 0
    },
    changeArgs(args) {
      this.widget.args.length = args
    },
    changeAlphaColors(colors) {
      this.widget.colors.length = colors
    },
    changeAuthContent(value) {
      if (this.widget.auth.startsWith('Basic '))
        this.widget.auth = 'Basic ' + btoa(value)
      else if (this.widget.auth.startsWith('Bearer '))
        this.widget.auth = 'Bearer ' + value
      else
        this.widget.auth = ''
    },
    changeAuth(auth) {
      this.auth = ''
      switch(auth) {
        case 0:
          this.widget.auth = ''
          break
        case 1:
          this.widget.auth = 'Basic '
          break
        case 2:
          this.widget.auth = 'Bearer '
          break
      }
    },
    changeBackground(background) {
      if (background)
        this.widget.transparent = false
    },
    changeText () {
      if (this.widget.type === 4)
        this.widget.content = this.widget.content.toUpperCase()
    },
    changeCert() {
      this.widget.cert = this.removeNonAscii(this.widget.cert)
      this.widget.cert = this.widget.cert.replaceAll('-----BEGIN CERTIFICATE-----', '').replaceAll('-----END CERTIFICATE-----', '')
      this.widget.cert = this.widget.cert.replaceAll(' ', '')
      if (!this.widget.cert)
        return
      let size = this.widget.cert.length
      let lines = Math.ceil(size / 64)
      for (let i = 0; i < lines; i++)
        this.widget.cert = [this.widget.cert.slice(0, i + i * 64), '\n', this.widget.cert.slice(i + i * 64)].join('')
      this.widget.cert = '-----BEGIN CERTIFICATE-----' + this.widget.cert
      this.widget.cert += '\n-----END CERTIFICATE-----\n'
    },
    changeShape() {
      if (this.widget.contentType === 2)
        this.widget.height = this.widget.width
      this.widget.colors.length = this.widget.contentType === 3 ? 0 : 1
      if (this.widget.colors.length > 0 && !this.widget.colors[0])
        this.widget.colors[0] = "0xFF0000"
      if (this.widget.contentType === 3) {
        this.widget.width = 1
        this.widget.height = 1
        this.widget.transparent = false
      }
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
      else if (this.widget.type === 8) {
        if (this.widget.contentType === 3)
          return []
        return ['Shape']
      }
      else
        return ['Text']
    }
  },
  mounted() {
    this.dynamic = this.widget.frequency !== 0
    this.args = this.widget.args.length
    this.alphaColors = this.widget.colors.length
  },
  data: () => ({
    dynamic: false,
    auth: '',
    args: 0,
    alphaColors: 0,
    collapsed: false,
    deleting: false
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

.variable-field {
  width: 200px;
}

.image-field {
  width: 170px;
}

.auth-select {
  width: 110px
}

.get-arg {
  width: 150px
}

.duration-field {
  width: 80px
}

.text-content-field {
  width: 300px;
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
