require 'graphics'
require './vec'

CANVAS_WIDTH = 500
CANVAS_HEIGHT = 500
TIMESTEP = 10 # this is fake idk yet

class Particle
  attr_reader :position, :prev_position
  def initialize(position, prev_position=Position.new(0, 0))
    @position = position
    @prev_position = prev_position
  end
end

class Edge
  attr_reader :start_point, :end_point, :rest_length
  def initialize(start_point, end_point)
    @start_point = start_point
    @end_point = end_point
    @rest_length = 100
  end
end

class Position
  attr_reader :x, :y
  def initialize(x_position, y_position)
    @x = x_position
    @y = y_position
  end
end

class Curtain
  attr_reader :points, :lines
  def initialize()
    @points = []

    points << Particle.new(Position.new(100, 100))
    points << Particle.new(Position.new(400, 100))

    @lines = [Edge.new(points[0], points[1])]

  end

  def reposition
    m_x = [points.length] # Current positions
    m_oldx = [points.length] # Previous positions

    points.length.times do |i|
      x = points[i].position.x
      temp = x
      oldx = points[i].prev_position.x
      a = 1 # lul
      x += x - oldx + a * TIMESTEP * TIMESTEP
      oldx = temp
    end
  end

  # // Verlet integration step
  # void ParticleSystem::Verlet() {
  #   for(int i=0; i<NUM_PARTICLES; i++) {
  #     Vector3& x = m_x[i];
  #     Vector3 temp = x;
  #     Vector3& oldx = m_oldx[i];
  #     Vector3& a = m_a[i];
  #     x += x - oldx + a * fTimeStep * fTimeStep;
  #     oldx = temp;
  #   }
  # }

  def satisfy_constraints
    20.times do
      lines.length.times do |i|
        e = lines[i]
        x1 = e.start_point.position.x
        # puts "first_x1: #{x1}"
        x2 = e.end_point.position.x
        # puts "first_x2: #{x2}"
        delta = x2 - x1
        # puts "delta: #{delta}"
        delta_length = Math.sqrt(delta * delta)
        # puts "delta_length: #{delta_length}"
        diff = (delta_length - e.rest_length) / delta_length
        # puts "diff: #{diff}"
        x1 += delta * 0.5 * diff
        # puts "second_x1: #{x1}"
        x2 -= delta * 0.5 * diff
        # puts "second_x2: #{x2}"
      end
    end
  end

# // Assume that an array of constraints, m_constraints, exists
# void ParticleSystem::SatisfyConstraints() {
#   for(int j=0; j<NUM_ITERATIONS; j++) {
#     for(int i=0; i<NUM_CONSTRAINTS; i++) {
#       Constraint& c = m_constraints[i];
#       Vector3& x1 = m_x[c.particleA];
#       Vector3& x2 = m_x[c.particleB];
#       Vector3 delta = x2-x1;
#       float deltalength = sqrt(delta*delta);
#       float diff=(deltalength-c.restlength)/deltalength;
#       x1 -= delta*0.5*diff;
#       x2 += delta*0.5*diff;
#     }
#     // Constrain one particle of the cloth to origo
#     m_x[0] = Vector3(0,0,0);
#   }
# }

  def update
    reposition
    satisfy_constraints
  end

end


class ClothSimulation < Graphics::Simulation
  attr_reader :sim
  def initialize
    super CANVAS_WIDTH, CANVAS_HEIGHT, 16, "Cloth"
    @sim = Curtain.new
    register_color(:purple, 114, 0, 127)
    register_color(:eggwhite, 245, 245, 232)
  end

  def update dt
    sim.update
  end

  def draw dt
    particle_size = 5

    clear(:eggwhite)
    sim.points.each do |point|
      self.circle point.position.x, point.position.y, particle_size, :purple, true
    end

    sim.lines.each do |line|
      self.line line.start_point.position.x, line.start_point.position.y,
                line.end_point.position.x, line.end_point.position.y, :purple
    end
  end
end

ClothSimulation.new.run
