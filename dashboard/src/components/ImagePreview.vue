<template>
  <div>
    <div>
      <canvas v-bind:style="{width: `${canvasWidth}px`, height: `${canvasHeight}px`}"
              :width="canvasWidth" :height="canvasHeight" ref="canvas"></canvas>
    </div>

    <div class="progress-bar" v-if="progressBar && this.$store.state.loadingImage === image">
      <md-progress-bar class="md-accent" md-mode="indeterminate"></md-progress-bar>
    </div>
  </div>
</template>

<script>
export default {
  name: "ImagePreview",
  props: ['image', 'frame', 'frames', 'pixelSize', 'progressBar', 'interval', 'canvasHeight', 'canvasWidth'],
  data: () => ({
    canvas: null,
    width: 64,
    height: 32,
    drawn: false,
    offset: 0,
    timer: null
  }),
  watch: {
    image: function (val) {
      if (!val || val.length === 0)
        return
      this.waitForImage(val).then(() => {
        this.reset(this.$store.state.images[val])
      })
    },
    frame: function (val) {
      this.handlePropChange()
    },
    frames: function (val) {
      this.handlePropChange()
    },
    interval: function (val) {
      this.handlePropChange()
    }
  },
  methods: {
    handlePropChange() {
      this.sleep(1).then(() => {
        this.reset(this.$store.state.images[this.image])
      })
    },
    reset(image) {
      if (this.timer)
        clearInterval(this.timer)
      if (this.frames > 1 && this.interval > 0)
        this.timer = setInterval(this.animate, this.interval)
      this.offset = 0
      this.height = this.$store.state.imageData[this.image].height
      this.width = this.$store.state.imageData[this.image].width
      this.drawImage(image)
    },
    animate() {
      this.offset++
      if (this.offset > this.frames - 1)
        this.offset = 0
      this.drawImage(this.$store.state.images[this.image])
    },
    drawImage(data) {
      if (!data) {
        console.warn('No data to draw')
        return
      }
      for (let y = 0; y < this.height; y++)
        for (let x = 0; x < this.width; x++) {
          this.canvas.fillStyle = data[(this.frame + this.offset) * this.width * this.height + y * this.width + x]
          this.canvas.fillRect(x * this.pixelSize, y * this.pixelSize, this.pixelSize, this.pixelSize)
        }
      this.drawn = true
    }
  },
  async mounted() {
    this.canvas = this.$refs['canvas'].getContext("2d")
    await this.waitForImageData()
    if (!this.image || this.image.length === 0)
      return
    await this.waitForImage(this.image)
    this.reset(this.$store.state.images[this.image])
  },
  created() {
    this.unsubscribe = this.$store.subscribe((mutation, state) => {
      if (mutation.type === 'setImage' && mutation.payload[0] === this.image)
        this.reset(mutation.payload[1])
    });
  },
  beforeDestroy() {
    this.unsubscribe();
    clearInterval(this.timer)
  }
}
</script>

<style scoped>
.progress-bar {
  margin-top: 2px;
  height: 5px;
}
</style>
