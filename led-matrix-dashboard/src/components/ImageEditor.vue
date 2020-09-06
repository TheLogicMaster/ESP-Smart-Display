<template>
  <div>
    <md-card>
      <md-card-header>
        <div class="md-title">
          Editing <i> {{ $route.params.name | remove_P }}</i>
        </div>
        <div>
          <md-button class="md-accent md-raised" @click="reload(true)">Reload</md-button>
          <md-button :disabled="$route.params.name.endsWith('_P')" class="md-accent md-raised" @click="saveClick">Save</md-button>
          <md-button class="md-accent md-raised" @click="saveAsPrompt = true">Save As</md-button>
          <md-button class="md-accent md-raised" @click="clear">Clear</md-button>
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
            <div v-for="i in 5" class="color" v-bind:style="[{'border-color': selectedPalette === i ? '#17a412' : '#6e5226'}, {'background-color': palette[i - 1]}]" @click="selectColor(i)" @dblclick="$refs.colorPicker.click()"></div>
          </div>
          <md-button class="md-icon-button" @click="redo">
            <md-icon>redo</md-icon>
          </md-button>
        </div>
      </md-card-header>
      <md-card-content>
        <canvas @click="click" class="canvas" ref="canvas" :width="canvasWidth" :height="canvasHeight"></canvas>
      </md-card-content>
    </md-card>

    <md-dialog-prompt
        :md-active.sync="saveAsPrompt"
        v-model="saveAsName"
        md-title="Enter new image name"
        md-input-maxlength="30"
        md-input-placeholder="Enter image name..."
        md-confirm-text="Done"
        @md-confirm="saveAs"/>
    <md-dialog-confirm
        :md-active.sync="saveConfirm"
        md-title="Save Image"
        md-content="Are you sure you want to overwrite the existing image?"
        md-confirm-text="Save"
        md-cancel-text="Cancel"
        @md-confirm="save($route.params.name)" />
    <md-dialog-alert
        :md-active.sync="errorAlert"
        :md-content="errorAlertText"
        md-confirm-text="Okay" />
  </div>
</template>

<script>
export default {
name: "ImageEditor",
  data: () => ({
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
    errorAlertText: ''
  }),
  methods: {
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
    async reload() {
      await this.getImage(this.$route.params.name)
      this.pixels = this.$store.state.images[this.$route.params.name]
      this.frame = 0
      this.frames = this.$store.state.imageData[this.$route.params.name].length
      this.history = []
      this.historyIndex = -1
      await this.drawImage()
    },
    showError(text) {
      this.errorAlert = true
      this.errorAlertText = text
    },
    saveAs(name) {
      if (name === '') {
        this.showError('Invalid image name')
        return
      }
      this.save(name)
      this.saveAsName = ''
    },
    saveClick() {
      this.saveConfirm = true
    },
    save(name) {
      let buffer = new Uint16Array(this.width * this.height * this.frames)
      for (let y = 0; y < this.height; y++)
        for (let x = 0; x < this.width; x++)
          buffer[y * this.width + x] = this.pack565Color(this.hexToRgb(this.getColorAt(x, y)))

      let data = new FormData()
      let blob = new Blob([buffer], {type: 'application/octet-stream'})
      let file = new File([blob], 'image.bin')
      data.append('image', file)
      this.$axios.post('/uploadImage', data, {headers: {'Content-Type': `multipart/form-data`}, params: {name: name, width: this.width, height: this.height, length: this.frames, type: this.$store.state.imageData[this.$route.params.name].type}}).then(response => {

      }).catch(error => {
        console.error(error)
      })
    },
    clear() {
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
          this.drawPixel(x, y, this.pixels[x + y * this.width])
    },
    selectColor(id) {
      this.color = this.palette[id - 1]
      this.selectedPalette = id
    },
    changeColor() {
      this.palette[this.selectedPalette - 1] = this.color
      this.$forceUpdate()
    },
    keyupHandler (event) {
      if (event.ctrlKey && event.code === 'KeyZ')
        this.undo()
      else if (event.ctrlKey && event.code === 'KeyY')
        this.redo()
    }
  },
  async mounted() {
  this.canvas = this.$refs['canvas'].getContext("2d")
    if (!(this.$route.params.name in this.$store.state.imageData))
      await this.waitForPromiseSuccess(this.getImageData, 500)
    while (!(this.$route.params.name in this.$store.state.images)) {
      await this.getImage(this.$route.params.name)
      await this.sleep(500)
    }
    this.pixels = this.$store.state.images[this.$route.params.name]
    this.frames = this.$store.state.imageData[this.$route.params.name].length
    await this.drawImage()
    document.addEventListener('keyup', this.keyupHandler)
  },
  destroyed() {
    document.removeEventListener('keyup', this.keyupHandler)
  }
}
</script>

<style scoped>
.canvas {
  //background-color: #4faf12;
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
