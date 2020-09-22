<template>
  <div>
    <md-card>
      <md-card-header>
        <div class="md-title">
          Editing <i> {{ $route.params.name | remove_P }}</i>
        </div>
        <div>
          <md-button class="md-accent md-raised" @click="reload(true)">Reload</md-button>
          <md-button
              :disabled="$store.state.imageData[$route.params.name] && $store.state.imageData[$route.params.name].progmem"
              class="md-accent md-raised" @click="saveConfirm = true">Save
          </md-button>
          <md-button class="md-accent md-raised" @click="saveAsPrompt = true">Save As</md-button>
          <md-button class="md-accent md-raised" @click="clear">Clear</md-button>
          <md-button
              :disabled="$store.state.imageData[$route.params.name] && $store.state.imageData[$route.params.name].progmem"
              class="md-accent md-raised" @click="editorDeleteImage">Delete
          </md-button>
          <md-button class="md-icon-button" @click="downloadDialog = true">
            <md-icon>save_alt</md-icon>
          </md-button>
        </div>
        <div>
          <md-button class="md-icon-button" @click="undo">
            <md-icon>undo</md-icon>
          </md-button>
          <input ref="colorPicker" v-model="color" type="color" hidden @change="changeColor"/>
          <md-field class="color-field">
            <label v-bind:style="{color: color}">Color</label>
            <md-input style="width: 100px" v-model="color" @click="$refs.colorPicker.click()"></md-input>
          </md-field>
          <div class="palette">
            <div v-for="i in 5" class="color"
                 v-bind:style="[{'border-color': selectedPalette === i ? '#17a412' : '#6e5226'}, {'background-color': palette[i - 1]}]"
                 @click="selectColor(i)" @dblclick="$refs.colorPicker.click()"></div>
          </div>
          <md-button class="md-icon-button" @click="redo">
            <md-icon>redo</md-icon>
          </md-button>
        </div>
        <div>
          <md-button class="md-icon-button" @click="frameBack">
            <md-icon>arrow_back</md-icon>
          </md-button>
          <md-button class="md-icon-button" @click="frameForward">
            <md-icon>arrow_forward</md-icon>
          </md-button>
        </div>
      </md-card-header>

      <div v-if="loading">
        Loading...
        <md-progress-spinner md-mode="indeterminate"></md-progress-spinner>
      </div>
      <div v-else-if="missing">
        Image not Found
      </div>

      <md-card-content class="canvas">
        <canvas @click="click" ref="canvas" :width="canvasWidth" :height="canvasHeight"></canvas>
      </md-card-content>
    </md-card>

    <md-dialog :md-active.sync="downloadDialog">
      <md-dialog-title>Download Image</md-dialog-title>
      <div>
        <md-radio v-model.number="downloadType" :value="0">Binary File(Current Frame)</md-radio>
        <md-radio v-model.number="downloadType" :value="1">Binary File(Entire Image)</md-radio>
        <md-radio v-model.number="downloadType" :value="2">565 Header File</md-radio>
        <md-radio v-model.number="downloadType" :value="3">Gimp Header File</md-radio>
        <md-radio v-model.number="downloadType" :value="4">PNG File</md-radio>
        <md-radio v-model.number="downloadType" :value="5">JPG File</md-radio>
        <md-radio v-model.number="downloadType" :value="6">BMP File</md-radio>
      </div>
      <md-dialog-actions>
        <md-button class="md-accent md-raised" @click="downloadDialog = false">Cancel</md-button>
        <md-button class="md-accent md-raised" @click="download">Download</md-button>
      </md-dialog-actions>
    </md-dialog>
    <md-dialog-prompt
        :md-active.sync="saveAsPrompt"
        v-model="saveAsName"
        md-title="Enter name to save as"
        md-input-maxlength="30"
        md-input-placeholder="Enter image name..."
        md-confirm-text="Save"
        md-cancel-text="Cancel"
        @md-confirm="saveAs"/>
    <md-dialog-confirm
        :md-active.sync="saveConfirm"
        md-title="Save Image"
        md-content="Are you sure you want to overwrite the existing image?"
        md-confirm-text="Save"
        md-cancel-text="Cancel"
        @md-confirm="save($route.params.name)"/>
    <md-dialog-alert
        :md-active.sync="errorAlert"
        :md-content="errorAlertText"
        md-confirm-text="Okay"/>
  </div>
</template>

<script>
export default {
  name: "ImageEditor",
  data: () => ({
    missing: false,
    loading: true,
    pixels: [],
    palette: ['#ff0000', '#00ff00', '#0000ff', '#ffffff', '#000000'],
    selectedPalette: 1,
    color: '#ff0000',
    tool: 0,
    width: 0,
    height: 0,
    canvasWidth: 0,
    canvasHeight: 0,
    canvas: null,
    frame: 0,
    frames: 1,
    pixelSize: 5,
    history: [],
    historyIndex: -1,
    saveAsPrompt: false,
    saveAsName: '',
    saveConfirm: false,
    errorAlert: false,
    errorAlertText: '',
    downloadDialog: false,
    downloadType: 0
  }),
  methods: {
    async editorDeleteImage() {
      await this.deleteImage(this.$route.params.name)
      await this.$router.push('/images')
    },
    coordsToPixels(x, y) {
      return {
        x: Math.floor((x - 1) / (this.pixelSize + 1)),
        y: Math.floor((y - 1) / (this.pixelSize + 1))
      }
    },
    pixelsToCoords(x, y) {
      return {
        x: 1 + x * (1 + this.pixelSize),
        y: 1 + y * (1 + this.pixelSize)
      }
    },
    getColorAt(x, y) {
      let coords = this.pixelsToCoords(x, y)
      let data = this.canvas.getImageData(coords.x, coords.y, 1, 1)
      return this.rgbToHex(data.data[0], data.data[1], data.data[2])
    },
    isNewHistorySame(entry1, entry2) {
      return entry1.x === entry2.x && entry1.y === entry2.y && entry1.colorOld === entry2.colorNew
    },
    click(event) {
      let coords = this.coordsToPixels(event.offsetX, event.offsetY)
      let entry = {x: coords.x, y: coords.y, colorOld: this.getColorAt(coords.x, coords.y), colorNew: this.color}
      this.drawPixel(coords.x, coords.y, this.color)
      if (entry.colorOld !== this.color && (this.historyIndex === -1 || !this.isNewHistorySame(entry, this.history[this.historyIndex]))) {
        this.historyIndex++;
        this.history = this.history.slice(0, this.historyIndex)
        this.history[this.historyIndex] = entry
        if (this.history.length > 100) {
          this.history.shift()
          this.historyIndex--
        }
      }
    },
    undo() {
      if (this.historyIndex >= 0) {
        let entry = this.history[this.historyIndex]
        this.drawPixel(entry.x, entry.y, entry.colorOld)
        this.historyIndex--
      }
    },
    redo() {
      if (this.historyIndex < this.history.length - 1) {
        this.historyIndex++
        let entry = this.history[this.historyIndex]
        this.drawPixel(entry.x, entry.y, entry.colorNew)
      }
    },
    frameForward() {
      if (this.frame < this.frames - 1) {
        this.writePixels()
        this.frame++
        this.drawImage()
      }
    },
    frameBack() {
      if (this.frame > 0) {
        this.writePixels()
        this.frame--
        this.drawImage()
      }
    },
    async reload() {
      this.loading = true
      await this.getImage(this.$route.params.name)
      this.pixels = this.$store.state.images[this.$route.params.name].slice(0)
      this.frame = 0
      this.frames = this.$store.state.imageData[this.$route.params.name].length
      this.history = []
      this.historyIndex = -1
      this.loading = false
      await this.drawImage()
    },
    showError(text) {
      this.errorAlert = true
      this.errorAlertText = text
    },
    async saveAs(name) {
      if (name === '') {
        this.showError('Invalid image name')
        return
      }
      await this.save(name)
      this.saveAsName = ''
    },
    formatUint16Color(color) {
      let c = '0x'
      let value = color.toString(16)
      for (let i = value.length; i < 4; i++)
        c += '0'
      c += value
      return c
    },
    async download() {
      this.downloadDialog = false
      this.writePixels()
      switch (this.downloadType) {
        case 0:
          let name = this.$route.params.name
          let bufferFrame = new Uint16Array(this.width * this.height)
          for (let i = 0; i < bufferFrame.length; i++)
            bufferFrame[i] = this.pack565Color(this.hexToRgb(this.pixels[this.frame * this.width * this.height + i]))
          this.$download(bufferFrame, `${name}-${this.width}-${this.height}-1.bin`)
          break
        case 1:
          let buffer = new Uint16Array(this.width * this.height * this.frames)
          for (let i = 0; i < buffer.length; i++)
            buffer[i] = this.pack565Color(this.hexToRgb(this.pixels[i]))
          this.$download(buffer, `${this.$route.params.name}-${this.width}-${this.height}-${this.frames}.bin`)
          break
        case 2:
        case 3:
          let string = `// Image Width: ${this.width} Pixels\n`
          string +=
              `// Image Height: ${this.height} Pixels\n` +
              `// Image Length: ${this.frames} Frames\n` +
              '\n' +
              '#include <Arduino.h>\n' +
              '\n' +
              '#if defined(__AVR__)\n' +
              '    #include <avr/pgmspace.h>\n' +
              '#elif defined(__PIC32MX__)\n' +
              '    #define PROGMEM\n' +
              '#elif defined(__arm__)\n' +
              '    #define PROGMEM\n' +
              '#endif\n' +
              '\n'
          if (this.downloadType === 2) {
            string += `uint16_t ${this.$route.params.name}_image[${this.width * this.height * this.frames}] = {\n`
            for (let i = 0; i < this.width * this.height * this.frames; i++) {
              string += this.formatUint16Color(this.pack565Color(this.hexToRgb(this.pixels[i]))) + ','
              if (i % 16 === 15)
                string += '\n'
            }
            if (!string.endsWith('\n'))
              string += '\n'
            string += '};'
          } else {
            string += 'char *taz_image =\n'
            for (let i = 0; i < this.width * this.height * this.frames; i++) {
              if (i % 16 === 0)
                string += '\t\"'
              string += this.encodeGimpColor(this.pixels[i])
              if (i % 16 === 15)
                string += '\"\n'
            }
            if (!string.endsWith('\n'))
              string += '\"'
            string += ';'
          }
          let blob = new Blob([string])
          this.$download(blob, this.$route.params.name + '.h')
          break
        default:
          let mimeType = [this.$jimp.MIME_PNG, this.$jimp.MIME_JPEG, this.$jimp.MIME_BMP][this.downloadType - 4]
          let image = await this.$jimp.create(this.width, this.height)
          for (let y = 0; y < this.height; y++)
            for (let x = 0; x < this.width; x++)
              image.setPixelColor(this.$jimp.cssColorToHex(this.getColorAt(x, y)), x, y)
          try {
            let result = await image.getBase64Async(mimeType)
            this.$download(result, this.$route.params.name + '.' + mimeType.slice(6), mimeType)
          } catch (error) {
            console.error(error)
          }
          break
      }
    },
    writePixels() {
      for (let y = 0; y < this.height; y++)
        for (let x = 0; x < this.width; x++)
          this.pixels[this.frame * this.width * this.height + y * this.width + x] = this.getColorAt(x, y)
    },
    async save(name) {
      this.writePixels()
      let buffer = new Uint16Array(this.width * this.height * this.frames)
      for (let i = 0; i < buffer.length; i++)
        buffer[i] = this.pack565Color(this.hexToRgb(this.pixels[i]))
      await this.saveImageBuffer(name, this.$store.state.imageData[this.$route.params.name], buffer)
    },
    clear() {
      this.history = []
      this.historyIndex = -1
      for (let y = 0; y < this.height; y++)
        for (let x = 0; x < this.width; x++)
          this.drawPixel(x, y, this.color)
    },
    drawPixel(x, y, color) {
      this.canvas.fillStyle = color
      this.canvas.fillRect(1 + (1 + this.pixelSize) * x, 1 + (1 + this.pixelSize) * y, this.pixelSize, this.pixelSize)
    },
    async drawImage() {
      this.width = this.$store.state.imageData[this.$route.params.name].width
      this.height = this.$store.state.imageData[this.$route.params.name].height
      this.canvasWidth = 1 + this.width * (this.pixelSize + 1)
      this.canvasHeight = 1 + this.height * (this.pixelSize + 1)
      await this.sleep(1)
      this.canvas.fillStyle = '#000000'
      this.canvas.clearRect(0, 0, this.canvasWidth, this.canvasHeight)

      for (let i = 0; i < this.width; i++)
        this.canvas.fillRect(i * (this.pixelSize + 1), 0, 1, this.canvasHeight)
      this.canvas.fillRect(this.canvasWidth - 1, 0, 1, this.canvasHeight)
      for (let i = 0; i < this.height; i++)
        this.canvas.fillRect(0, i * (this.pixelSize + 1), this.canvasWidth, 1)
      this.canvas.fillRect(0, this.canvasHeight - 1, this.canvasWidth, 1)

      for (let y = 0; y < this.height; y++)
        for (let x = 0; x < this.width; x++)
          this.drawPixel(x, y, this.pixels[this.frame * this.width * this.height + x + y * this.width])
    },
    selectColor(id) {
      this.color = this.palette[id - 1]
      this.selectedPalette = id
    },
    changeColor() {
      this.palette[this.selectedPalette - 1] = this.color
      this.$forceUpdate()
    },
    keyupHandler(event) {
      if (event.ctrlKey && event.code === 'KeyZ')
        this.undo()
      else if (event.ctrlKey && event.code === 'KeyY')
        this.redo()
    }
  },
  async mounted() {
    this.encodeGimpColor(this.decodeGimpImage('-G37'))
    if (!this.$store.state.imageData.hasOwnProperty(this.$route.params.name))
      await this.waitForPromiseSuccess(this.getImageData)
    if (!this.$store.state.imageData.hasOwnProperty(this.$route.params.name)) {
      this.loading = false
      this.missing = true
      return
    }
    while (!this.$store.state.images.hasOwnProperty(this.$route.params.name)) {
      await this.getImage(this.$route.params.name)
      await this.sleep(500)
    }
    this.canvas = this.$refs['canvas'].getContext("2d")
    this.pixels = this.$store.state.images[this.$route.params.name].slice(0)
    this.frames = this.$store.state.imageData[this.$route.params.name].length
    await this.drawImage()
    document.addEventListener('keyup', this.keyupHandler)
    this.loading = false
  },
  destroyed() {
    document.removeEventListener('keyup', this.keyupHandler)
  }
}
</script>

<style scoped>
.md-radio {
  display: flex;
}

.md-card {
  min-width: 600px;
}

.canvas {
  width: 512px;
  display: inline-block;
}

.color {
  width: 30px;
  height: 30px;
  display: inline-block;
  border-style: solid;
}

.color-field {
  width: 100px;
  display: inline-block;
}

.palette {
  height: 34px;
  border-style: solid;
  display: inline-block;
}
</style>
