<template>
  <div>
    <div class="image">
      <canvas :width="width * pixelSize" :height="height * pixelSize" ref="canvas"></canvas>
    </div>

    <div class="progress-bar">
      <md-progress-bar v-if="this.$store.state.loadingImage === image" class="md-accent" md-mode="indeterminate"></md-progress-bar>
    </div>
  </div>
</template>

<script>
export default {
  name: "ImagePreview",
  props: ['image'],
  data: () => ({
    canvas: null,
    pixelSize: 3,
    width: 64,
    height: 32,
    drawn: false
  }),
  methods: {
    drawImage(data) {
      this.height = parseInt(this.$store.state.imageData[this.image]["height"])
      this.width = parseInt(this.$store.state.imageData[this.image]["width"])
      this.canvas.clearRect(0, 0, this.width * this.pixelSize, this.height * this.pixelSize)
      for (let y = 0; y < this.height; y++)
        for (let x = 0; x < this.width; x++) {
          this.canvas.fillStyle = data[y * this.width + x]
          this.canvas.fillRect(x * this.pixelSize, y * this.pixelSize, this.pixelSize , this.pixelSize)
        }
      this.drawn = true
    }
  },
  async mounted() {
    this.canvas = this.$refs['canvas'].getContext("2d")
    while(!this.$store.state.images.hasOwnProperty(this.image))
      await this.sleep(500)
    this.drawImage(this.$store.state.images[this.image])
  },
  created() {
    this.unsubscribe = this.$store.subscribe((mutation, state) => {
      if (mutation.type === 'setImage' && mutation.payload[0] === this.image)
        this.drawImage(mutation.payload[1])
    });
  },
  beforeDestroy() {
    this.unsubscribe();
  }
}
</script>

<style scoped>
.image {
  margin-bottom: 2px;
}
.progress-bar {
  height: 5px;
}
</style>
